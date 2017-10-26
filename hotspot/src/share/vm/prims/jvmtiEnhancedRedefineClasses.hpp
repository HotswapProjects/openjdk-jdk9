/*
 * Copyright (c) 2003, 2016, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SHARE_VM_PRIMS_JVMTIREDEFINECLASSES2_HPP
#define SHARE_VM_PRIMS_JVMTIREDEFINECLASSES2_HPP

#include "jvmtifiles/jvmtiEnv.hpp"
#include "memory/oopFactory.hpp"
#include "memory/resourceArea.hpp"
#include "oops/objArrayKlass.hpp"
#include "oops/objArrayOop.hpp"
#include "runtime/vm_operations.hpp"
#include "gc/shared/vmGCOperations.hpp"



class VM_EnhancedRedefineClasses: public VM_GC_Operation {
 private:
  // These static fields are needed by ClassLoaderDataGraph::classes_do()
  // facility and the AdjustCpoolCacheAndVtable helper:
  static Array<Method*>* _old_methods;
  static Array<Method*>* _new_methods;
  static Method**      _matching_old_methods;
  static Method**      _matching_new_methods;
  static Method**      _deleted_methods;
  static Method**      _added_methods;
  static int             _matching_methods_length;
  static int             _deleted_methods_length;
  static int             _added_methods_length;
  static Klass*          _the_class_oop;

  // The instance fields are used to pass information from
  // doit_prologue() to doit() and doit_epilogue().
  jint                        _class_count;
  const jvmtiClassDefinition *_class_defs;  // ptr to _class_count defs

  // This operation is used by both RedefineClasses and
  // RetransformClasses.  Indicate which.
  JvmtiClassLoadKind          _class_load_kind;

  // _index_map_count is just an optimization for knowing if
  // _index_map_p contains any entries.
  int                         _index_map_count;
  intArray *                  _index_map_p;

  // _operands_index_map_count is just an optimization for knowing if
  // _operands_index_map_p contains any entries.
  int                         _operands_cur_length;
  int                         _operands_index_map_count;
  intArray *                  _operands_index_map_p;

  GrowableArray<Klass*>*      _scratch_classes;
  jvmtiError                  _res;

  // Enhanced class redefinition
  GrowableArray<Klass*>*      _affected_klasses;
  int                         _max_redefinition_flags;

  // Performance measurement support. These timers do not cover all
  // the work done for JVM/TI RedefineClasses() but they do cover
  // the heavy lifting.
  elapsedTimer  _timer_rsc_phase1;
  elapsedTimer  _timer_rsc_phase2;
  elapsedTimer  _timer_vm_op_prologue;

  // These routines are roughly in call order unless otherwise noted.

  // Load the caller's new class definition(s) into _scratch_classes.
  // Constant pool merging work is done here as needed. Also calls
  // compare_and_normalize_class_versions() to verify the class
  // definition(s).
  jvmtiError load_new_class_versions(TRAPS);

  // Searches for all affected classes and performs a sorting such tha
  // a supertype is always before a subtype.
  jvmtiError find_sorted_affected_classes(TRAPS);

  jvmtiError do_topological_class_sorting(TRAPS);

  jvmtiError find_class_bytes(instanceKlassHandle the_class, const unsigned char **class_bytes, jint *class_byte_count, jboolean *not_changed);
  int calculate_redefinition_flags(instanceKlassHandle new_class);
  void calculate_instance_update_information(Klass* new_version);

  void rollback();
  static void mark_as_scavengable(nmethod* nm);
  static void unpatch_bytecode(Method* method);

  // Verify that the caller provided class definition(s) that meet
  // the restrictions of RedefineClasses. Normalize the order of
  // overloaded methods as needed.
  jvmtiError compare_and_normalize_class_versions(
    instanceKlassHandle the_class, instanceKlassHandle scratch_class);

  // Figure out which new methods match old methods in name and signature,
  // which methods have been added, and which are no longer present
  void compute_added_deleted_matching_methods();

  // Change jmethodIDs to point to the new methods
  void update_jmethod_ids();

  // In addition to marking methods as old and/or obsolete, this routine
  // counts the number of methods that are EMCP (Equivalent Module Constant Pool).
  int check_methods_and_mark_as_obsolete();
  void transfer_old_native_function_registrations(instanceKlassHandle the_class);

  // Install the redefinition of a class
  void redefine_single_class(Klass* scratch_class_oop, TRAPS);

  void swap_annotations(instanceKlassHandle new_class,
                        instanceKlassHandle scratch_class);

  // Increment the classRedefinedCount field in the specific InstanceKlass
  // and in all direct and indirect subclasses.
  void increment_class_counter(InstanceKlass *ik, TRAPS);

  // Support for constant pool merging (these routines are in alpha order):
  void append_entry(const constantPoolHandle& scratch_cp, int scratch_i,
    constantPoolHandle *merge_cp_p, int *merge_cp_length_p, TRAPS);
  void append_operand(const constantPoolHandle& scratch_cp, int scratch_bootstrap_spec_index,
    constantPoolHandle *merge_cp_p, int *merge_cp_length_p, TRAPS);
  void finalize_operands_merge(const constantPoolHandle& merge_cp, TRAPS);
  int find_or_append_indirect_entry(const constantPoolHandle& scratch_cp, int scratch_i,
    constantPoolHandle *merge_cp_p, int *merge_cp_length_p, TRAPS);
  int find_or_append_operand(const constantPoolHandle& scratch_cp, int scratch_bootstrap_spec_index,
    constantPoolHandle *merge_cp_p, int *merge_cp_length_p, TRAPS);
  int find_new_index(int old_index);
  int find_new_operand_index(int old_bootstrap_spec_index);
  bool is_unresolved_class_mismatch(const constantPoolHandle& cp1, int index1,
    const constantPoolHandle& cp2, int index2);
  void map_index(const constantPoolHandle& scratch_cp, int old_index, int new_index);
  void map_operand_index(int old_bootstrap_spec_index, int new_bootstrap_spec_index);
  bool merge_constant_pools(const constantPoolHandle& old_cp,
    const constantPoolHandle& scratch_cp, constantPoolHandle *merge_cp_p,
    int *merge_cp_length_p, TRAPS);
  jvmtiError merge_cp_and_rewrite(instanceKlassHandle the_class,
    instanceKlassHandle scratch_class, TRAPS);
  u2 rewrite_cp_ref_in_annotation_data(
    AnnotationArray* annotations_typeArray, int &byte_i_ref,
    const char * trace_mesg, TRAPS);
  bool rewrite_cp_refs(instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_annotation_struct(
    AnnotationArray* class_annotations, int &byte_i_ref, TRAPS);
  bool rewrite_cp_refs_in_annotations_typeArray(
    AnnotationArray* annotations_typeArray, int &byte_i_ref, TRAPS);
  bool rewrite_cp_refs_in_class_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_element_value(
    AnnotationArray* class_annotations, int &byte_i_ref, TRAPS);
  bool rewrite_cp_refs_in_type_annotations_typeArray(
    AnnotationArray* type_annotations_typeArray, int &byte_i_ref,
    const char * location_mesg, TRAPS);
  bool rewrite_cp_refs_in_type_annotation_struct(
    AnnotationArray* type_annotations_typeArray, int &byte_i_ref,
    const char * location_mesg, TRAPS);
  bool skip_type_annotation_target(
    AnnotationArray* type_annotations_typeArray, int &byte_i_ref,
    const char * location_mesg, TRAPS);
  bool skip_type_annotation_type_path(
    AnnotationArray* type_annotations_typeArray, int &byte_i_ref, TRAPS);
  bool rewrite_cp_refs_in_fields_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  void rewrite_cp_refs_in_method(methodHandle method,
    methodHandle * new_method_p, TRAPS);
  bool rewrite_cp_refs_in_methods(instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_methods_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_methods_default_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_methods_parameter_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_class_type_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_fields_type_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  bool rewrite_cp_refs_in_methods_type_annotations(
    instanceKlassHandle scratch_class, TRAPS);
  void rewrite_cp_refs_in_stack_map_table(const methodHandle& method, TRAPS);
  void rewrite_cp_refs_in_verification_type_info(
         address& stackmap_addr_ref, address stackmap_end, u2 frame_i,
         u1 frame_size, TRAPS);
  void set_new_constant_pool(ClassLoaderData* loader_data,
         instanceKlassHandle scratch_class,
         constantPoolHandle scratch_cp, int scratch_cp_length, TRAPS);

  void flush_dependent_code(instanceKlassHandle k_h, TRAPS);

  // lock classes to redefine since constant pool merging isn't thread safe.
  void lock_classes();
  void unlock_classes();

  static void dump_methods();

  // Check that there are no old or obsolete methods
  class CheckClass : public KlassClosure {
    Thread* _thread;
   public:
    CheckClass(Thread* t) : _thread(t) {}
    void do_klass(Klass* k);
  };

  // Unevolving classes may point to methods of the_class directly
  // from their constant pool caches, itables, and/or vtables. We
  // use the ClassLoaderDataGraph::classes_do() facility and this helper
  // to fix up these pointers.
  class ClearCpoolCacheAndUnpatch : public KlassClosure {
    Thread* _thread;
   public:
    ClearCpoolCacheAndUnpatch(Thread* t) : _thread(t) {}
    void do_klass(Klass* k);
  };

  // Clean MethodData out
  class MethodDataCleaner : public KlassClosure {
   public:
    MethodDataCleaner() {}
    void do_klass(Klass* k);
  };
 public:
  VM_EnhancedRedefineClasses(jint class_count,
                     const jvmtiClassDefinition *class_defs,
                     JvmtiClassLoadKind class_load_kind);
  VMOp_Type type() const { return VMOp_RedefineClasses; }
  bool doit_prologue();
  void doit();
  void doit_epilogue();

  bool allow_nested_vm_operations() const        { return true; }
  jvmtiError check_error()                       { return _res; }

  // Modifiable test must be shared between IsModifiableClass query
  // and redefine implementation
  static bool is_modifiable_class(oop klass_mirror);

  // Error printing
  void print_on_error(outputStream* st) const;
};
#endif // SHARE_VM_PRIMS_JVMTIREDEFINECLASSES2_HPP
