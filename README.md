DCEVM & HotswapJDK
==============================
In contrast to standard Java, where the hotswap is limited to in-body code changes, the DCEVM + HotswapAgent allow following code changes:

* Add/remove/modify class fields.
* Add/remove/modify methods. 
* Add/remove/modify annotations
* Add/remove/modify classes including anonymous classes. 
* Add/remove static member of classes. 
* Add/remove enum values

The only unsupported change is hierarchy change (change superclass).


DCEVM changed files (overview)
==============================
TODO - add more detail.

makefiles -> JVM_GetVmMemoryPressure

src/vm/classfile
----------------

share/vm/ci ->
  int find(Metadata* key, GrowableArray<ciMetadata*>* objects);
  bool is_found_at(int index, Metadata* key, GrowableArray<ciMetadata*>* objects);
  void insert(int index, ciMetadata* obj, GrowableArray<ciMetadata*>* objects);
replaced by:
    static int metadata_compare(Metadata* const& key, ciMetadata* const& elt);

src/vm/classfile
----------------

classFileParser -> new variable _pick_newest introduced

classLoaderData -> do_unloading - fix unloaded for DCEVM

dictionary ->
 - update_klass() -> update class entry in the Dictionary
 - rollback_redefinition() -> rever all class entries to old_version() in the Dictionary
 - do_unloading() -> do not unload redefined class - fix for DCEVM may be needed

javaClasses -> 
  - new interface to java.lang.invoke.DirectMethodHandle$StaticAccessor and java.lang.invoke.DirectMethodHandle$Accessor

loadConstraints -> update_after_redefinition - switch to newest classes

systemDictionary ->
  - check that only newest class is used
  - new method remove_from_hierarchy() introduced

verifier -> use newest class

vmSymbols -> java.lang.invoke.DirectMethodHandle$StaticAccessor and java.lang.invoke.DirectMethodHandle$Accessor

src/vm/gc
----------------
cms - not supported for DCEVM
g1 -  not supported for DCEVM
serial - **** DCEVM *****

shared - **** DCEVM *****
 - rescue()
 - must_rescue()


Universe ->
  - _is_redefining_gc_run
  - root_oops_do() - iterate over all objects

src/vm/oops
----------------
cache - clear_entries, disable field modification checks
instanceKlass - update_jmethod_id, implements_interface_any_version, ...
klass ->
  - new redefinition flags - _old_version, _new_version, _redefinition_flags, _is_redefining,
  - update_information - store mapping betwen old fields and new fields - use to recalculate objects
method - oldMethod, newMethod, ..
  - remove_from_sibling_list - fix list of subclasses after superclass change

src/vm/prims
----------------
jvmtiEnhancedRedefineClasses - **** DCEVM *****
jvmtiEnv -> switch to enhanced version
jvmtiGetLoadedClasses -> iterate only on newest classes
jvmtiImpl -> method breakpoints in old method version
methodHandles -> reinit - FIXME

src/vm/runtime
----------------
arguments -> force usage of Serial GC
globals -> new flag AllowEnhancedClassRedefinition
reflection -> use new class version
