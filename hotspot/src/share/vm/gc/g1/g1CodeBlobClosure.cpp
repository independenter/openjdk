/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "code/nmethod.hpp"
#include "gc/g1/g1CodeBlobClosure.hpp"
#include "gc/g1/g1CollectedHeap.inline.hpp"
#include "gc/g1/heapRegion.hpp"
#include "gc/g1/heapRegionRemSet.hpp"
#include "oops/oop.inline.hpp"

template <typename T>
void G1CodeBlobClosure::HeapRegionGatheringOopClosure::do_oop_work(T* p) {
  _work->do_oop(p);
  T oop_or_narrowoop = oopDesc::load_heap_oop(p);
  if (!oopDesc::is_null(oop_or_narrowoop)) {
    oop o = oopDesc::decode_heap_oop_not_null(oop_or_narrowoop);
    HeapRegion* hr = _g1h->heap_region_containing_raw(o);
    assert(!_g1h->obj_in_cs(o) || hr->rem_set()->strong_code_roots_list_contains(_nm), "if o still in collection set then evacuation failed and nm must already be in the remset");
    hr->add_strong_code_root(_nm);
  }
}

void G1CodeBlobClosure::HeapRegionGatheringOopClosure::do_oop(oop* o) {
  do_oop_work(o);
}

void G1CodeBlobClosure::HeapRegionGatheringOopClosure::do_oop(narrowOop* o) {
  do_oop_work(o);
}

void G1CodeBlobClosure::do_code_blob(CodeBlob* cb) {
  nmethod* nm = cb->as_nmethod_or_null();
  if (nm != NULL) {
    if (!nm->test_set_oops_do_mark()) {
      _oc.set_nm(nm);
      nm->oops_do(&_oc);
      nm->fix_oop_relocations();
    }
  }
}

