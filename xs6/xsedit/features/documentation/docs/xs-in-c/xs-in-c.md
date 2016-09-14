<!--
|     Copyright (C) 2010-2016 Marvell International Ltd.
|     Copyright (C) 2002-2010 Kinoma, Inc.
|
|     Licensed under the Apache License, Version 2.0 (the "License");
|     you may not use this file except in compliance with the License.
|     You may obtain a copy of the License at
|
|      http://www.apache.org/licenses/LICENSE-2.0
|
|     Unless required by applicable law or agreed to in writing, software
|     distributed under the License is distributed on an "AS IS" BASIS,
|     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
|     See the License for the specific language governing permissions and
|     limitations under the License.
-->
<!-- Version: 151214a-CR

This document describes XS in C, the C interface to `xslib`, the runtime library component of the XS toolkit.
-->

# XS in C

## About This Document

This document describes XS in C, the C interface to `xslib`, the runtime library component of the XS toolkit. Details about XS are provided in the companion document [*XS*](../xs). 

In accordance with the ECMAScript specifications, `xslib` implements only generic features that all scripts can use. An application defines the specific features that its own scripts can use through C callbacks. An application that uses `xslib` is a host in ECMAScript terminology. 

This document is organized into the following sections:

* [Slots](#slots) describes how to handle ECMAScript constructs in C callbacks, with examples that show the correspondences between ECMAScript and XS in C. 
 
* [Machine](#machine) introduces the main structure of `xslib` (its virtual machine) and explains how to use `xslib` to build a host step by step and make C callbacks available to scripts. It includes examples that combine to make a simple command-line tool.

* A [Glossary](#glossary) includes all the terms defined in this document (or used here and defined in the [*XS*](../xs) document).
 
### Licensing Information

This document is part of the XS runtime library, `xslib`. `xslib` is free software; you can redistribute it or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation--either version 2 of the License or (at your option) any later version.

`xslib` is distributed in the hope that it will be useful, but without any warranty--without even the implied warranty of merchantability or fitness for a particular purpose. See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with `xslib`; if not, write to:

>Free Software Foundation, Inc.  
59 Temple Place, Suite 330  
Boston, MA 02111-1307 USA


<a id="slots"></a>
##Slots

In `xslib`, everything is stored in *slots.* A slot is an opaque structure that is manipulated only through XS in C.

<div class="ccode"></div>

	typedef struct xsSlotRecord xsSlot
	struct xsSlotRecord {
		void* data[4];
	};

There are seven types of slot:

 <!--From CR: I added lots of <div> tags like the following; please verify that it's in fact C code in all such places (remove tag if not).-->
 
<div class="ccode"></div>

	enum {
		xsUndefinedType,
		xsNullType,
		xsBooleanType,
		xsIntegerType,
		xsNumberType,
		xsStringType,
		xsReferenceType 
	}
	typedef char xsType;

The undefined, null, boolean, number, and string slots correspond to the ECMAScript primitive types. The integer slot is an optimization; for scripts, it is equivalent to the number slot. The reference slot corresponds to the ECMAScript `reference` type.

The `xsTypeOf` macro returns the type of a slot. It is similar to the ECMAScript `typeof` keyword.

<!--From CR: This document's structure is not what it should be, with the overview-type info combined with reference-type info (like the following). Ideally they would be separated as in similar Kinoma docs. Chris says this may be OK because this doc is very technical and not for the average developer. However, any developer might prefer having API details in a separate section for later reference. Just food for thought for possible future development of this doc.-->
 
`xsType xsTypeOf(xsSlot theSlot)`

| | 
| --- | 
| `theSlot`| 

> The slot to test

| |
| --- |
| Returns| 

> The type of the slot

#####In ECMAScript:

	switch(typeof arguments[0]) {
		case "undefined": break;
		/* Null is an object. */
		case "boolean": break;	
		/* Integers are numbers */	
		case "number": break;
		case "string": break;
		case "object": break;
		case "function": break;
	}	

#####In C:

<div class="ccode"></div>

	switch(xsTypeOf(xsArg(0))) {
		case xsUndefinedType: break;
		case xsNullType: break;
		case xsBooleanType: break;
		case xsIntegerType: break;
		case xsNumberType: break;
		case xsStringType: break;
		case xsReferenceType: break;  /* Objects and functions are references. */
	}		


###Primitives

The undefined, null, boolean, integer, number, and string slots (collectively known as *direct slots*) correspond to the ECMAScript primitive types, with the integer slot added as an optimization.

The undefined and null slots contain no value. The `xsUndefined` and `xsNull` macros return slots of those types.

`xsSlot xsUndefined`

| |
| --- |
| Returns| 

> An undefined slot

`xsSlot xsNull`

| |
| --- |
| Returns| 

> A null slot

The remaining direct slots contain values of the corresponding type.

<div class="ccode"></div>

	typedef char xsBooleanValue;  
	typedef long xsIntegerValue;  
	typedef double xsNumberValue;  
	typedef char* xsStringValue;`

The following macros return slots of each of these types (set to a particular value) or access the value in a slot. When accessing the value in a slot, you specify a desired type; the slot is coerced to the requested type if necessary, and the value is returned. The string macros listed here are discussed further at the end of the list.

`xsSlot xsTrue`

| |
| --- |
| Returns| 

> A boolean slot containing `true`

`xsSlot xsFalse`

| |
| --- |
| Returns| 

> A boolean slot containing `false`

`xsSlot xsBoolean(xsBooleanValue theValue)`

| | 
| --- | 
| `theValue`| 

> The value to be contained in the slot

| |
| --- |
| Returns| 

> A boolean slot

`xsBooleanValue xsToBoolean(xsSlot theSlot)`

| | 
| --- | 
| `theSlot`| 

> The slot to coerce to boolean

| |
| --- |
| Returns| 

> The value contained in the slot

`xsSlot xsInteger(xsIntegerValue theValue)`

| | 
| --- | 
| `theValue`| 

> The value to be contained in the slot

| |
| --- |
| Returns| 

> An integer slot

`xsIntegerValue xsToInteger(xsSlot theSlot)`

| | 
| --- | 
| `theSlot`| 

> The slot to coerce to integer

| |
| --- |
| Returns| 

> The value contained in the slot

`xsSlot xsNumber(xsNumberValue theValue)`

| | 
| --- | 
| `theValue`| 

> The value to be contained in the slot

| |
| --- |
| Returns| 

> A number slot

`xsNumberValue xsToNumber(xsSlot theSlot)`

| | 
| --- | 
| `theSlot`| 

> The slot to coerce to number

| |
| --- |
| Returns| 

> The value contained in the slot

`xsSlot xsString(xsStringValue theValue)`

| | 
| --- | 
| `theValue`| 

> The value to be contained in the slot

| |
| --- |
| Returns| 

> A string slot

`xsStringValue xsToString(xsSlot theSlot)`

| | 
| --- | 
| `theSlot`| 

> The slot to coerce to string

| |
| --- |
| Returns| 

> The string contained in the slot

`xsStringValue xsToStringBuffer(xsSlot theSlot, xsStringValue theBuffer, xsIntegerValue theSize)`

| | 
| --- | 
| `theSlot`| 

> The slot to coerce to string

| | 
| --- | 
| `theBuffer`| 

> A buffer to copy the string into

| | 
| --- | 
| `theSize`| 

> The size of the buffer

| |
| --- |
| Returns| 

> The buffer containing the copy of the string

`xsStringValue xsToStringCopy(xsSlot theSlot)`


| | 
| --- | 
| `theSlot`| 

> The slot to coerce to string

| |
| --- |
| Returns| 

> A buffer (created by `malloc`) containing a copy of the string

A string value is a pointer to a UTF-8 C string. The virtual machine and the garbage collector of `xslib` manage all UTF-8 C strings used by scripts. C constants, C globals, or C locals can safely be passed to the `xsString` macro, since it duplicates its parameter. 

For the sake of performance, the `xsToString` macro returns the value contained in the slot itself. Since `xslib` can compact string values at any time, the result of the `xsToString` macro cannot be used across or in other macros of XS in C and cannot be modified in place.

To get a copy of the string value, use the `xsToStringBuffer` or `xsToStringCopy` macro. With the `xsToStringBuffer` macro, the buffer provided has to be large enough to hold a copy of the string value. With the `xsToStringCopy` macro, the copy is created by `malloc`; use `free` to delete it. 

#####In ECMAScript:

	undefined
	null
	false
	true
	0
	0.0
	"foo"

#####In C:

<div class="ccode"></div>

	xsUndefined;
	xsNull;
	xsFalse;
	xsTrue;
	xsInteger(0);
	xsNumber(0.0);
	xsString("foo")
 
###Instances and Prototypes

In XS in C as in ECMAScript, an object can inherit properties from another object, which can inherit from another object, and so on; the inheriting object is the *instance*, and the object from which it inherits is the *prototype*.

Reference slots (type `xsReferenceType`) are *indirect* slots: they contain a reference to an instance of an object, function, array, and so on. Instances themselves are made of slots that are the properties of the instance (or, for an array, the items of the instance).

An instance shares or overrides the properties of its prototype, as follows:

- When the property of an instance is accessed, the property is searched for in the instance. If the instance has the property, it is returned; otherwise the property is searched for in the prototype of the instance, and so on up the prototype hierarchy, until there is no prototype, in which case `xsUndefined` is returned.

- When the property of an instance is assigned a value, the property is searched for in the instance. If the instance has the property, it is set; otherwise the property is inserted into the instance.

- When the property of an instance is deleted, the property is searched for in the instance. If the instance has the property, the property is removed from the instance.

XS in C defines the following macros to refer to the prototypes created by `xslib`:

`xsSlot xsObjectPrototype`   
`xsSlot xsFunctionPrototype`   
`xsSlot xsArrayPrototype`   
`xsSlot xsDatePrototype`   
`xsSlot xsRegExpPrototype`  
`xsSlot xsErrorPrototype`

| |
| --- |
| Returns | 

> A reference to the prototype of object, function, array, date, regular expression, or error instances, respectively.

To create an instance, use the `xsNewInstanceOf` macro and pass the prototype, which can be a prototype defined by `xslib` or an instance created by the host application.

`xsSlot xsNewInstanceOf(xsSlot thePrototype)`

| | 
| --- | 
| `thePrototype`| 

> A reference to the prototype of the instance to create

| | 
| --- | 
| Returns | 

> A reference to the new instance 


To test whether an instance has a particular prototype, directly or indirectly (that is, one or more levels up in the prototype hierarchy), use the `xsIsInstanceOf` macro.

`xsBooleanValue xsIsInstanceOf(xsSlot theInstance, xsSlot thePrototype)`

| | 
| --- | 
| `theInstance`| 

> A reference to the instance to test

| | 
| --- | 
| `thePrototype`| 

> A reference to the prototype to test

| | 
| --- | 
| Returns | 

> `true` if the instance has the prototype, `false` otherwise

The `xsNewInstanceOf` and `xsIsInstanceOf` macros have no equivalent in ECMAScript; scripts create or test instances through *constructors* rather than directly through prototypes. A constructor is a function that has a `prototype` property and is used to create instances with `new` or to test instances with `instanceof`. (There is also a way to invoke constructors in XS in C, as described in the section [xsNew*](#xsnew).)

#####In ECMAScript:

	if (xsThis instanceof Object)
		return new Object();

#####In C:

<div class="ccode"></div>

	if (xsIsInstanceOf(xsThis, xsObjectPrototype))
		xsResult = xsNewInstanceOf(xsObjectPrototype);

###Identifiers

In ECMAScript, the properties of an object are identified by strings, and the items of an array are identified by numbers. In XS in C, on the other hand, all identifiers are indexes. 

<div class="ccode"></div>

	typedef short xsIndex;

The properties of an object are identified by negative indexes, and the items of an array are identified by positive indexes.

<a id="xsid"></a>
####xsID

XS in C defines the `xsID` macro to convert a string value corresponding to an ECMAScript property name into an identifier. 

`xsIndex xsID(xsStringValue theValue)`

| | 
| --- | 
| `theValue` | 

> The string to convert

| |
| --- |
| Returns | 

> The identifier (a negative index)

Given the same virtual machine, the same string value is always converted to the same identifier, so frequently used identifiers can be cached.

In the C examples below, the `xsGet` macro (discussed in the next section) takes as its second argument the identifier of the property or item to get.

#####In ECMAScript:

	this.foo
	this[0]

#####In C:

<div class="ccode"></div>

	xsGet(xsThis, xsID("foo"));
	xsGet(xsThis, 0);

####xsIsID

You can test whether a given string corresponds to an existing property name with the `xsIsID` macro (although this macro is rarely used).

`xsBooleanValue xsIsID(xsStringValue theValue)`

| | 
| --- | 
| `theValue` | 

> The string to test

| |
| --- |
| Returns | 

> `true` if the string is an identifier, `false` otherwise


### Properties 

This section describes the macros related to accessing properties of objects (or items of arrays), as summarized in Table 1.

**Table 1.** Property-Related Macros

<table class="normalTable">
  <tbody>
    <tr>
      <th scope="col">Macro</th>
      <th scope="col">Description</th>
    </tr>
    <tr>
      <td><code>xsGlobal</code></td>
      <td>Returns a special instance made of global properties available to scripts</td>
    </tr> 
    <tr>
      <td><code>xsHas</code></td>
      <td>Tests whether an instance has a particular property</td>
    </tr> 
    <tr>
      <td><code>xsGet</code></td>
      <td>Gets a property or item of an instances</td>
    </tr> 
    <tr>
      <td><code>xsSet</code></td>
      <td>Sets a property or item of an instance</td>
    </tr> 
    <tr>
      <td><code>xsDelete</code></td>
      <td>Deletes a property or item of an instance</td>
    </tr> 
    <tr>
      <td><code>xsCall0</code> ... <code>xsCall7</code></td>
      <td>Calls the function referred to by a property or item of an instance</td>
    </tr> 
    <tr>
      <td><code>xsNew0</code> ... <code>xsNew7</code></td>
      <td>Calls the constructor referred to by a property or item of an instance</td>
    </tr>     
  </tbody>
</table>

#### xsGlobal

Globals available to scripts are just properties of a special instance. XS in C defines the `xsGlobal` macro to refer to such a special instance.

`xsSlot xsGlobal`

| |
| --- |
| Returns | 

> A reference to the special instance made of globals

You can use the `xsGet`, `xsSet`, `xsDelete`, `xsCall*`, and `xsNew*` macros with the `xsGlobal` macro as the first parameter. Examples are shown in the sections describing those macros.

#### xsHas

To test whether an instance has a property corresponding to a particular ECMAScript property name, use the `xsHas` macro. This macro is similar to the ECMAScript `in` keyword.

`xsBooleanValue xsHas(xsSlot theThis, xsIndex theIndex)`

| |
| --- |
| `theThis` | 

> A reference to the instance to test


| |
| --- |
| `theIndex` | 

> The identifier of the property to test


| |
| --- |
| Returns | 

> `true` if the instance has the property, `false` otherwise


#####In ECMAScript:

	if ("foo" in this)

#####In C:

<div class="ccode"></div>

	if (xsHas(xsThis, xsID("foo")));


#### xsGet

To get a property or item of an instance, use the `xsGet` macro. If the property or item is not defined by the instance or its prototypes, this macro returns `xsUndefined`.

`xsSlot xsGet(xsSlot theThis, xsIndex theIndex)`

| |
| --- |
| `theThis` | 

> A reference to the instance that has the property or item


| |
| --- |
| `theIndex` | 

> The identifier of the property or item to get


| |
| --- |
| Returns | 

> A slot containing what is contained in the property or item


#####In ECMAScript:

	foo
	this.foo
	this[0]

#####In C:

<div class="ccode"></div>

	xsGet(xsGlobal, xsID("foo"));
	xsGet(xsThis, xsID("foo"));
	xsGet(xsThis, 0);

#### xsSet

To set a property or item of an instance, use the `xsSet` macro. If the property or item is not defined by the instance, this macro inserts it into the instance.

`void xsSet(xsSlot theThis, xsIndex theIndex, xsSlot theParam)`

| |
| --- |
| `theThis` | 

> A reference to the instance that will have the property or item


| |
| --- |
| `theIndex` | 

> The identifier of the property or item to set


| |
| --- |
| Returns | 

> A slot containing what will be contained in the property or item


#####In ECMAScript:

	foo = 0
	this.foo = 1
	this[0]

#####In C:

<div class="ccode"></div>

	xsSet(xsGlobal, xsID("foo"), xsInteger(0));
	xsSet(xsThis, xsID("foo"), xsInteger(1));
	xsSet(xsThis, 0, xsInteger(2));


#### xsDelete

To delete a property or item of an instance, use the `xsDelete` macro. If the property or item is not defined by the instance, this macro has no effect.

`void xsDelete(xsSlot theThis, xsIndex theIndex)`

| |
| --- |
| `theThis` | 

> A reference to the instance that has the property or item


| |
| --- |
| `theIndex` | 

> The identifier of the property or item to delete


#####In ECMAScript:

	delete foo
	delete this.foo
	delete this[0]

#####In C:

<div class="ccode"></div>

	xsDelete(xsGlobal, xsID("foo"));
	xsDelete(xsThis, xsID("foo"));
	xsDelete(xsThis, 0);

#### xsCall*

When a property or item of an instance is a reference to a function, you can call the function with one of the `xsCall*` macros (where `*` is `0` through `7`, representing the number of parameter slots passed). If the property or item is not defined by the instance or its prototypes or is not a reference to a function, the `xsCall*` macro throws an exception.

`xsSlot xsCall0(xsSlot theThis, xsIndex theIndex)`
  
`xsSlot xsCall1(xsSlot theThis, xsIndex theIndex, xsSlot theParam0)`
 
`xsSlot xsCall2(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1)`

`xsSlot xsCall3(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2)`

`xsSlot xsCall4(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3)`

`xsSlot xsCall5(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4)`

`xsSlot xsCall6(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5)`

`xsSlot xsCall7(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5, xsSlot theParam6)`

| |
| --- |
| `theThis` | 

> A reference to the instance that will have the property or item


| |
| --- |
| `theIndex` | 

> The identifier of the property or item to call


| |
| --- |
| `theParam0` ... `theParam6` | 

> The parameter slots to pass to the function

| |
| --- |
| Returns | 

> The result slot of the function


#####In ECMAScript:

	foo()
	this.foo(1)
	this[0](2, 3)

#####In C:

<div class="ccode"></div>

	xsCall0(xsGlobal, xsID("foo"));
	xsCall1(xsThis, xsID("foo"), xsInteger(1));
	xsCall2(xsThis, 0, xsInteger(2), xsInteger(3));

<a id="xsnew"></a>
#### xsNew*

When a property or item of an instance is a reference to a constructor, you can call the constructor with one of the `xsNew*` macros (where `*` is `0` through `7`, representing the number of parameter slots passed). If the property or item is not defined by the instance or its prototypes or is not a reference to a constructor, the `xsNew*` macro throws an exception.

`xsSlot xsNew0(xsSlot theThis, xsIndex theIndex)`

`xsSlot xsNew1(xsSlot theThis, xsIndex theIndex, xsSlot theParam0)`

`xsSlot xsNew2(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1)`

`xsSlot xsNew3(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2)`

`xsSlot xsNew4(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3)`

`xsSlot xsNew5(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4)`

`xsSlot xsNew6(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5)`

`xsSlot xsNew7(xsSlot theThis, xsIndex theIndex, xsSlot theParam0, xsSlot theParam1, xsSlot theParam2, xsSlot theParam3, xsSlot theParam4, xsSlot theParam5, xsSlot theParam6)`

| |
| --- |
| `theThis` | 

> A reference to the instance that has the property or item


| |
| --- |
| `theIndex` | 

> The identifier of the property or item to call

| |
| --- |
| `theParam0` ... `theParam6` | 

> The parameter slots to pass to the constructor


| |
| --- |
| Returns | 

> The result slot of the constructor


#####In ECMAScript:

	new foo()
	new this.foo(1)
	new this[0](2, 3)

#####In C:

<div class="ccode"></div>

	xsNew0(xsGlobal, xsID("foo"));
	xsNew1(xsThis, xsID("foo"), xsInteger(1));
	xsNew2(xsThis, 0, xsInteger(2), xsInteger(3));


#### xsTest

Like an `if` clause in ECMAScript, the `xsTest` macro takes a value of any type and determines whether it is true or false. This macro applies the same rules as in ECMAScript (per the ECMA-262 specification, section 12.5).

`xsBooleanValue xsTest(xsSlot theValue)`

| |
| --- |
| `theValue` | 

> The value to test


| |
| --- |
| Returns | 

> `true` if the value is true, `false` otherwise


#####In ECMAScript:

	if (foo) {}

#####In C:

<div class="ccode"></div>

	if (xsTest(xsGet(xsGlobal, xsID("foo"))) {}
	

### Arguments and Variables

The virtual machine of `xslib` uses a heap and a stack of slots. With XS in C, you can access stack slots directly and heap slots indirectly, through references.

When a C callback is executed, the stack contains its argument slots, its `this` slot, and its result slot, but no variable slots. To use variable slots, you have to reserve them on the stack with the `xsVars` macro at the beginning of the callback execution. 

`void xsVars(xsIntegerValue theCount)`

| |
| --- |
| `theCount` | 

> The number of variable slots to reserve

Argument and variable slots are accessed and assigned by index. An exception is thrown if the index is invalid.

Initially:

- The argument slots are the parameter slots passed to the function or constructor.

- If the callback is a function, the `this` slot refers to the instance being called and the result slot is undefined.

- If the callback is a constructor, the `this` and result slots refer to the instance being created.

- The variable slots are undefined.

Scripts can call a constructor as a function or a function as a constructor. To find out whether the C callback is executed as a constructor or as a function, you can check whether the result slot is initially undefined.

`xsSlot xsArgc` 

| |
| --- |
| Returns |

> An integer slot that contains the number of arguments

`xsSlot xsArg(xsIntegerValue theIndex)`

| |
| --- |
| `theIndex` | 

> The index of the argument, from 0 to `xsArgc-1`

| |
| --- |
| Returns | 

> The argument slot

`xsSlot xsThis`

| |
| --- |
| Returns | 

> The `this` slot

`xsSlot xsResult`

| |
| --- |
| Returns | 

> The result slot

`xsSlot xsVarc`

| |
| --- |
| Returns | 

> An integer slot that contains the number of variables

`xsSlot xsVar(xsIntegerValue theIndex)`

| |
| --- |
| `theIndex` | 

> The index of the variable, from 0 to `xsVarc-1`

| |
| --- |
| Returns | 

> The variable slot

Usually you access the argument, `this`, result, and variable slots but you assign only the result and variable slots. Whatever is in the result slot at the end of the callback execution is returned to scripts by the function or constructor.

In the C example in this section (and the next one), `xsMachine` is the virtual machine structure, as shown in the section [Machine](#machine).

#####In ECMAScript:

	function foo() {
		var c, i, s;
		c = arguments.length;
		s = "";
		for (i = 0; i < c; i++)
			s = s.concat(arguments[i]);
		return s;
	}


#####In C:

<div class="ccode"></div>

	void xs_foo(xsMachine* the) {
		xsIntegerValue c, i;
		xsVars(1);
		c = xsToInteger(xsArgc));
		xsVar(0) = xsString("");
		for (i = 0; i < c; i++)
			xsVar(0) = xsCall1(xsVar(0), xsID("concat"), xsArg(i));
		xsResult = xsVar(0);
	}

### Garbage Collector

When `xslib` needs to create slots and there is not enough memory, it automatically deletes unused slots. To force `xslib` to delete useless slots, you can use the `xsCollectGarbage` macro.

`void xsCollectGarbage()`

The garbage collector of `xslib` uses a mark and sweep algorithm. If you store slots in a C global or a C allocated structure, use the `xsRemember` and `xsForget` macros to tell `xslib`.

`void xsRemember(xsSlot theSlot)`

| |
| --- |
| `theSlot` | 

> The slot to remember

`void xsForget(xsSlot theSlot)`

| |
| --- |
| `theSlot` | 

> The slot to forget

`xsRemember` links and `xsForget` unlinks a slot to and from a chain of slots. The garbage collector of `xslib` scans such a chain to mark the slots that the C global or the C allocated structure references.

#####In C:

<div class="ccode"></div>

	xsSlot gFooSlot;
	void xsSetupFoo(xsMachine* the) {
		gFooSlot = xsThis;
		xsRemember(gFooSlot);
	}
	void xsCleanupFoo(xsMachine* the) {
		xsForget(gFooSlot);
	}

### Exceptions

To handle exceptions in C, `xslib` uses `setjmp`, `longjmp`, and a chain of `jmp_buf` buffers, defined as follows:

<!--From CR: On the web page, only "stack" is highlighted (blue) in the following code and similar code in this doc; is something wrong?-->

<div class="ccode"></div>

	typedef struct xsJumpRecord xsJump
	struct xsJumpRecord {
		jmp_buf buffer;
		xsJump* nextJump;
		xsSlot* stack;
		xsSlot* frame;
	};

However, you never need to use any of this, because XS in C defines macros for throwing and catching exceptions.

To throw an exception, use the `xsThrow` macro.

`void xsThrow(xsSlot theException)`

| |
| --- |
| `theException` | 

> The exception slot

The `xsThrow` macro assigns the current exception; the `xsException` macro accesses the current exception.

`xsSlot xsException`

| |
| --- |
| Returns | 

> The exception slot

As shown in the following example, the `xsTry` and `xsCatch` macros must be used together to catch exceptions. If you catch an exception in your C callback and you want to propagate the exception to the script that calls your function or constructor, you have to throw the exception again.

#####In ECMAScript:

	{
		try {
			/* Exception thrown here ... */
		}
		catch(e) {
			/* ... is caught here. */	
			throw e
	 	}
	}

#####In C:

<div class="ccode"></div>

	 {
		xsTry {
			/* Exception thrown here ... */
		}
		xsCatch {
			/* ... is caught here. */
			xsThrow(xsException)
		}
	} 

### Errors

Most exceptions will be thrown by C callbacks. C callbacks are the interface between scripts and systems. A lot of system calls can fail, and they have a way to return an error to the application.

To throw exceptions in case of system call errors, use the `xsError` macro or, as a shortcut in the C callbacks, the `xsIfError` or `xsElseError` macro.

####xsError

`void xsError(xsIntegerValue theError)`

| |
| --- |
| `theError` | 

> The error number

The exception will be an instance of `Error` with a message based on `theError`.

#####In C:

<div class="ccode"></div>

	anError = FskMemPtrNew(1024, &aBuffer));
	if (anError) xsError(anError);

####xsIfError

The `xsIfError` macro is a shortcut for system calls that return 0 if they succeeded and a nonzero value if they failed.

`void xsIfError(xsIntegerValue theError)`

| |
| --- |
| `theError` | 

> If nonzero, triggers an exception

The exception will be an instance of `Error` with a message based on `theError`.

#####In C:

<div class="ccode"></div>

	xsIfError(FskMemPtrNew(1024, &aBuffer));
	xsIfError(FskFileRead(aFile, 1024, aBuffer, &aSize));

####xsElseError
The `xsElseError` macro is a shortcut for system calls that return 0 if they failed and a nonzero value if they succeeded. 

`void xsElseError(xsIntegerValue theAssertion)`

| |
| --- |
| `theAssertion` | 

> If 0, triggers an exception

The exception will be an instance of `Error` with a message based on `GetLastError` on Windows systems and on `errno` elsewhere.

#####In C:

<div class="ccode"></div>

	xsElseError(aBuffer = malloc(1024));
	xsElseError(ReadFile(aFile, aBuffer, 1024, &aSize, NULL));

### Debugger

XS in C provides two macros to help you debug your C callbacks.

The `xsDebugger` macro is equivalent to the ECMAScript `debugger` keyword.

`void xsDebugger()`

The `xsTrace` macro is equivalent to the ECMAScript `trace` function.

`void xsTrace(xsStringValue theMessage)`

| |
| --- |
| `theMessage` | 

> The message to log in the debugger

#####In ECMAScript:

<div class="ccode"></div>

	debugger;
	trace("Hello xsbug!\n");

#####In C:

<div class="ccode"></div>

	 xsDebugger();
	 xsTrace("Hello xsbug!\n");


<a id="machine"></a>
##Machine

The main structure of `xslib` is its virtual machine, which is what parses, compiles, links, and executes scripts. A virtual machine is a mostly opaque structure. Some members of the structure are available to optimize the macros of XS in C; you never need to use them.

<div class="ccode"></div>

	typedef struct xsMachineRecord xsMachine
	struct xsMachineRecord {
		xsSlot* stack;
		xsSlot* stackBotton;
		xsSlot* stackTop;
		xsSlot* frame;
		xsJump* firstJump;
	};

<!--From CR re next paragraph: You introduced "VM", an abbreviation this doc doesn't use, and I'm under the impression that this doc uses "machine" as short for "virtual machine"… but in that case this sentence wouldn't make sense, hence my attempting to distinguish the two machines. If this edit isn't OK, please change to some other way of distinguishing them (e.g., maybe "an xsMachine object for each virtual machine"?)-->

A single machine does not support multiple threads. To work with multiple threads, you can create an `xslib` machine, for each virtual machine, with the host optionally providing a way for the machines to communicate. 

<!--From CR re "callbacks are provided to read and write files" in next paragraph: Moved out of the explanation of the example below because I assume this is true of the basic sample app; correct?-->

This section describes the macros provided in XS in C for creating a machine, building a host, and other operations involved in creating an application. The examples in this section combine to make a simple command-line tool. Its first argument is the script to execute, and any additional arguments are available as elements of the `argv` array. C callbacks are provided to read and write files. In the following example of invoking the tool, the last two arguments specify source and destination files.

`TestTool toLowercase.js input.txt output.txt`

Suppose `toLowercase.js` contains this script:

	src = new File(argv[2], "r");
	dst = new File(argv[3], "w");
	while (buf = src.getLine())
		dst.putLine(buf.toLowercase()); 

The effect would be to convert the contents of `input.txt` (one line at a time) to lowercase and write the result to `output.txt`.

 You can use a simple example like this one as a basis to experiment, to test features of `xslib`, to provide features to scripts with XS in C, and so on.

<a id="machine-allocation"></a>
###Machine Allocation

To use `xslib` you have to create a machine with the `xsNewMachine` macro, allocating memory for it as required. Its parameters are:

-  A structure with members that are essentially parameters specifying what to allocate for the machine. Pass `NULL` if you want to use the defaults. 

<div class="ccode indentCode"></div>

	typedef struct {
		xsIntegerValue initialChunkSize;
		xsIntegerValue incrementalChunkSize;
		xsIntegerValue initialHeapCount;
		xsIntegerValue incrementalHeapCount;
		xsIntegerValue stackCount;
		xsIntegerValue symbolCount;
		xsIntegerValue symbolModulo;
	} xsAllocation;

- The grammar generated by `xsc` when you compile and link XS and C files into an application or a dynamically linked library (see the [*XS*](../xs) document for details). Grammars prototype ECMAScript instances and are used in parsing XML documents into ECMAScript instances and serializing ECMAScript instances into XML documents; pass `NULL` if you do not want any of these capabilities.

<div class="ccode indentCode"></div>

	typedef struct {
		xsCallback callback;
		xsStringValue symbols;
		xsIntegerValue symbolsSize;
		xsStringValue code;
		xsIntegerValue codeSize;
		xsStringValue name;
	} xsGrammar;


- A context you can set and get in your callbacks (as discussed in the next section). Pass `NULL` if you do not want any context initially.

`xsMachine* xsNewMachine(xsAllocation* theAllocation, xsGrammar* theGrammar, void* theContext)`

| |
| --- |
| `theAllocation` | 

> The parameters of the machine

| |
| --- |
| `theGrammar` | 

> The main grammar of the machine, or `NULL`

| |
| --- |
| `theContext` | 

> The initial context of the machine, or `NULL`

| |
| --- |
| Returns | 

> A machine if successful, otherwise `NULL`

Regarding the parameters of the machine that are specified in the `xsAllocation` structure:

- A machine manages strings and bytecodes in chunks. The initial chunk size is the initial size of the memory allocated to chunks. The incremental chunk size tells `xslib` how to expand the memory allocated to chunks. (Note that these chunks are unrelated to the chunk object, the XS extension to ECMAScript that enables access to binary data.)

- A machine uses a heap and a stack of slots. The initial heap count is the initial number of slots allocated to the heap. The incremental heap count tells `xslib` how to increase the number of slots allocated to the heap. The stack count is the number of slots allocated to the stack.

- The symbol count is the number of symbols the machine will use. The symbol modulo is the size of the hash table the machine will use for symbols. A symbol binds a string value and an identifier; see [`xsID`](#xs-id).

When you are done with a machine, you must free it with the `xsDeleteMachine` macro. The destructors of all the host objects are executed, and all the memory allocated by the machine is freed.

`void xsDeleteMachine(xsMachine* the)`

| |
| --- |
| `the` | 

> A machine

The `xsDeleteMachine` macro is one of a number of macros described in this document that have an explicit machine parameter named `the`, for which the value returned by `xsNewMachine` is passed. (The other such macros are `xsGetContext`, `xsSetContext`, `xsBuildHost`, `xsBeginHost`, `xsEndHost`, `xsExecute`, `xsParse`, and `xsParseBuffer`.) Only those macros have an explicit `the` parameter because they are the only ones that can be used outside a callback and cannot throw exceptions. Callbacks must name their machine parameter `the` because all other macros have an implicit parameter named `the`; the primary reason for this convention is terseness, but it also emphasizes the fact that these other macros can be used only inside a callback and can throw exceptions.

The following example illustrates the use of `xsNewMachine` and `xsDeleteMachine`. The `xsMainContext` function called in the example is defined in the next section.

#####Example

<div class="ccode"></div>

	int main(int argc, char* argv[]) 
	{
		xsAllocation anAllocation = {
			32 * 1024,    /* initialChunkSize */
			16 * 1024,    /* incrementalChunkSize */
			2048,         /* initialHeapCount */
			1024,         /* incrementalHeapCount */
			1024,         /* stackCount */
			4096,         /* symbolCount */
			1993          /* symbolModulo */
		};
		xsMachine* aMachine;

		aMachine = xsNewMachine(&anAllocation, NULL, NULL);
		if (aMachine) {
			xsMainContext(aMachine, argc, argv);
			xsDeleteMachine(aMachine);
		}
		else
			fprintf(stderr, "### Cannot allocate machine\n");
		return 0;
	}

###Context

The machine will use your C code mostly through callbacks. In your callbacks, you can set and get a *context*: a pointer to an area where you can store and retrieve information for the machine.

`void xsSetContext(xsMachine* the, void* theContext)`

| |
| --- |
| `the` | 

> A machine

| |
| --- |
| `theContext` | 

> A context


`void* xsGetContext(xsMachine* the)`

| |
| --- |
| `the` | 

> A machine

| |
| --- |
| Returns | 

> A context

The following code shows a context being set in the `xsMainContext` function, which was called in the preceding section's example. The `xsMainHost` function called by `xsMainContext` is defined later, in the example in the section [`xsNewHostProperty`](#xsNewHostProperty).


##### Example

<div class="ccode"></div>

	typedef struct {
		int argc;
		char** argv;
	} xsContext;

	void xsMainContext(xsMachine* theMachine, int argc, char* argv[])
	{
		xsContext* aContext;

		aContext = malloc(sizeof(xsContext));
		if (aContext) {
			aContext->argc = argc;
			aContext->argv = argv;
			xsSetContext(theMachine, aContext);
			xsMainHost(theMachine, argc, argv);
			xsSetContext(theMachine, NULL);
			free(aContext);
		}
		else
			fprintf(stderr, "### Cannot allocate context\n");
	}

An example of getting the machine context in a callback is shown later, in the section [`xsNewHostProperty`](#xsNewHostProperty).


###Host

This section describes the host-related macros of XS in C (see Table 2). The example code that uses these macros is shown after the last macro it uses has been described. (The remaining two macros do not enter into the sample application.)

**Table 2.** Host-Related Macros

<table class="normalTable">
  <tbody>
    <tr>
      <th scope="col">Macro</th>
      <th scope="col">Description</th>
    </tr>
    <tr>
      <td><code>xsGlobal</code></td>
      <td>Returns a special instance made of global properties available to scripts</td>
    </tr> 
    <tr>
      <td><code>xsBuildHost</code></td>
      <td>Sets up the stack, executes the callback you pass to build the host, and cleans up the stack</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsNewHostFunction</code></p>
        <p><code>xsNewHostConstructor</code></p>
      </td>
      <td>Creates a host function or host constructor</td>
    </tr> 
    <tr>
      <td><code>xsNewHostObject</code></td>
      <td>Creates a host object</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsGetHostData</code></p>
        <p><code>xsSetHostData</code></p>
      </td>
      <td>Gets or sets the data in a host object</td>
    </tr> 
    <tr>
      <td><code>xsSetHostDestructor</code></td>
      <td>Sets the destructor for a host object</td>
    </tr> 
    <tr>
      <td><code>xsNewHostProperty</code></td>
      <td>Creates a property of a host object (or creates a getter or setter)</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsBeginHost</code></p>
        <p><code>xsEndHost</code></p>
      </td>
      <td>Can be used together in place of <code>xsBuildHost</code> to set up and clean up the stack, respectively, so that you can use all the macros of XS in C in between</td>
    </tr> 
  </tbody>
</table>


#### xsBuildHost

Once you have a machine and, if desired, a context, you need to build the host with the `xsBuildHost` macro. This macro sets up the stack, executes the callback you pass (declared as shown below), and cleans up the stack. 

<div class="ccode"></div>

	typedef void (*xsCallback)(xsMachine* the);

As described in the following sections, it is in such a callback that you can create instances like host functions, host constructors, or host objects and make them available to scripts.

You can use the `xsBuildHost` macro at the beginning of your application to initialize the host, and during the execution of the application to change the host. 

`xsBooleanValue xsBuildHost(xsMachine* the, xsCallback theCallback)`

| |
| --- |
| `the` | 

> A machine

| |
| --- |
| `theCallback` | 

> A callback to execute to build the host

| |
| --- |
| Returns | 

> `true` if successful, `false` otherwise


#### xsNewHostFunction and xsNewHostConstructor

A *host function* is a special kind of function whose implementation is in C rather than ECMAScript. For a script, a host function is just like a function; however, when a script invokes a host function, a callback is executed. The same is true for *host constructors*.

<div class="ccode"></div>

	typedef void (*xsCallback)(xsMachine* the);

To create a host function, use the `xsNewHostFunction` macro.

`xsSlot xsNewHostFunction(xsCallback theCallback, xsIntegerValue theLength);`

| |
| --- |
| `theCallback` | 

> The callback to execute

| |
| --- |
| `theLength` | 

> The number of parameters expected by the callback

| |
| --- |
| Returns | 

> A reference to the new host function


To create a host constructor, use the `xsNewHostConstructor` macro.

`xsSlot xsNewHostConstructor(xsCallback theCallback,`
`	xsIntegerValue theLength, xsSlot thePrototype)`

| |
| --- |
| `theCallback` | 

> The callback to execute

| |
| --- |
| `theLength` | 

> The number of parameters expected by the callback

| |
| --- |
| `thePrototype` | 

> A reference to the prototype of the instance to create

| |
| --- |
| Returns | 

> A reference to the new host constructor


#### xsNewHostObject

A *host object* is a special kind of object with data that can be directly accessed only in C. The data in a host object is invisible to scripts. 

When the garbage collector is about to get rid of a host object, it executes the host object's destructor, if any. No reference to the host object is passed to the destructor: a destructor can only destroy data.

<div class="ccode"></div>

	typedef void (xsDestructor)(void* theData);

To create a host object, use the `xsNewHostObject` macro. Pass the host object's destructor, or `NULL` if it does not need a destructor. 

`xsSlot xsNewHostObject(xsDestructor theDestructor)`

| |
| --- |
| `theDestructor` | 

> The destructor to be executed by the garbage collector

| |
| --- |
| Returns | 

> A reference to the new host object


#### xsGetHostData and xsSetHostData

To get and set the data of a host object, use the `xsGetHostData` and `xsSetHostData` macros. Both throw an exception if the `theThis` parameter does not refer to a host object.

`void* xsGetHostData(xsSlot theThis)`

| |
| --- |
| `theThis` | 

> A reference to a host object

| |
| --- |
| Returns | 

> The data

`void xsSetHostData(xsSlot theThis, void* theData)`

| |
| --- |
| `theThis` | 

> A reference to a host object

| |
| --- |
| Returns | 

> The data

#### xsSetHostDestructor

To set the destructor of a host object (or to clear the destructor, by passing `NULL`), use the `xsSetHostDestructor` macro. This macro throws an exception if the `theThis` parameter does not refer to a host object.

`void xsSetHostDestructor(xsSlot theThis, xsDestructor theDestructor)`

| |
| --- |
| `theThis` | 

> A reference to a host object

| |
| --- |
| `theDestructor` | 

> The destructor to be executed by the garbage collector


<a id="xsNewHostProperty"></a>
#### xsNewHostProperty

To set properties of host objects, you can use the `xsNewHostProperty` macro. Like the `xsSet` macro, if the property is not defined by the instance, the `xsNewHostProperty` macro inserts it into the instance; however, unlike `xsSet`, `xsNewHostProperty` can set the attributes of the property, using one or more of the following constants. 

<div class="ccode"></div>

	enum {
		xsDefault = 0,
		xsDontDelete = 2,
		xsDontEnum = 4,
		xsDontScript = 8,
		xsDontSet = 16,
		xsIsGetter = 32,
		xsIsSetter = 64,
		xsChangeAll = 30
	} 
	typedef unsigned char xsAttribute;

`void xsNewHostProperty(xsSlot theThis, xsIndex theIndex, xsSlot theValue, xsAttribute theAttributes, xsAttribute theMasks)`

| |
| --- |
| `theThis` | 

> A reference to the instance that will have the property

| |
| --- |
| `theIndex` | 

> The identifier of the property to set

| |
| --- |
| `theValue` | 

> A slot containing what will be contained in the property

| |
| --- |
| `theAttributes` | 

> A combination of attributes to set if permitted by `theMasks`

| |
| --- |
| `theMasks` | 

> A combination of attributes to set or clear (per `theAttributes`)

For `theAttributes`, specify the constants corresponding to the attributes you want to set (the others being cleared); however, note that in order for those attributes to be set or cleared, you must also specify them in `theMasks`. In other words, only the attributes specified by `theMasks` will be set or cleared as indicated by `theAttributes`. Any attributes not specified in `theMasks` will be inherited from the instance's prototype.

The `xsDontDelete`, `xsDontEnum`, and `xsDontSet` attributes correspond to the ECMAScript `DontDelete`, `DontEnum`, and `ReadOnly` attributes. The `xsDontScript` attribute defines whether the property is visible or invisible to scripts. By default a property can be deleted, enumerated, and set, and can be used by scripts.

When a property is created, if the prototype of the instance has a property with the same name, its attributes are inherited; otherwise, by default, a property can be deleted, enumerated, and set, and can be used by scripts.

The `xsNewHostProperty` macro can also be used to create getters and setters; see the [*XS*](../xs) document for details). In that case `theValue` has to be a function, and both the `theAttributes` and `theMasks` parameters have to be `xsIsGetter` or `xsIsSetter`. 

The `xsMainExecute` function called in the following example is defined later, in the example in the section [Script Execution](#script-execution).

##### Example

<div class="ccode"></div>

	void xsMainHost(xsMachine* theMachine, int argc, char* argv[])`
	{
		if (xsBuildHost(theMachine, xsMainHostCallback))
			xsMainExecute(theMachine, argc, argv);
		else
			fprintf(stderr, "### Cannot build host\n");
	}

	void xsMainHostCallback(xsMachine* the)
	{
		xsContext* aContext = xsGetContext(the);
		int argi;

		/* Build an array with the arguments ... */
		xsResult = xsNewObject(xsArrayPrototype);
		for (argi = 0; argi < aContext->argc; argi++)
			xsSet(xsResult, argi, xsString(aContext->argv[argi]));
		/* ... and make it available as argv. */
		xsSet(xsGlobal, xsID("argv"), xsResult);
		
		/* Build a host object to embed a file ... */
		xsResult = xsNewHostObject(xs_FileDestructor);
		/* ... with properties to get and put lines ... */
		xsNewHostProperty(xsResult, xsID("getLine"), 
			xsNewHostFunction(xs_FileGetLine, 0),
			xsDontDelete | xsDontEnum | xsDontSet, xsChangeAll);
		xsNewHostProperty(xsResult, xsID("putLine"), 
			xsNewHostFunction(xs_FilePutLine, 0),
			xsDontDelete | xsDontEnum | xsDontSet, xsChangeAll);
		/* ... and make it available as a File constructor. */
		xsNewHostProperty(xsGlobal, xsID("File"), 
			xsNewHostConstructor(xs_File, 2, xsResult),
			xsDontDelete | xsDontEnum | xsDontSet, xsChangeAll);
	}

	void xs_File(xsMachine* the)
	{
		FILE* aFile = fopen(xsToString(xsArg(0)), xsToString(xsArg(1)));
		xsSetHostData(xsThis, aFile);
	}

	void xs_FileDestructor(void* theData)
	{
		if (theData)
			fclose(theData);
	}

	void xs_FileGetLine(xsMachine* the)
	{
		FILE* aFile = xsGetHostData(xsThis);
		char aLine[1024];
		if (fgets(aLine, sizeof(aLine)-1, aFile))
			xsResult = xsString(aLine);
	}

	void xs_FilePutLine(xsMachine* the)
	{
		FILE* aFile = xsGetHostData(xsThis);
		fputs(xsToString(xsArg(0)), aFile);
	}


#### xsBeginHost and xsEndHost

When it is inconvenient to retrieve and store information through the context--for instance, in a system callback--use the `xsBeginHost` and `xsEndHost` macros instead of the `xsBuildHost` macro. 

`void xsBeginHost(xsMachine* the)`

| |
| --- |
| `the` | 

> A machine

`void xsEndHost(xsMachine* the)`

| |
| --- |
| `the` | 

> A machine

`void xsEndHost(xsMachine* the)`

| |
| --- |
| `the` | 

> A machine

The `xsBeginHost` macro sets up the stack, and the `xsEndHost` macro cleans up the stack, so that you can use all the macros of XS in C in the block between `xsBeginHost` and `xsEndHost`.

##### Example

<div class="ccode"></div>

	long FAR PASCAL xsWndProc(HWND hwnd, UINT m, UINT w, LONG l)
	{
		long result = 0;
		xsMachine* aMachine = GetWindowLongPtr(hwnd, GWL_USERDATA);
		xsBeginHost(aMachine);
		{
			result = xsToInteger(xsCall3(xsGlobal, xsID("dispatch"), 
				xsInteger(m), xsInteger(w), xsInteger(l)));
		} 
		xsEndHost(aMachine);
		return result;
	}

<a id="script-execution"></a>
### Script Execution

Once you have created a machine and built a host, you can execute scripts with the `xsExecute` macro. This macro takes a stream and a getter. 

<div class="ccode"></div>

	typedef int (*xsGetter)(void* theStream);

For example, if the script is a file, the stream can be declared as `FILE*` and the getter can be `fgetc` (although the getter can be anything else with equivalent behavior).
 
`xsBooleanValue xsExecute(xsMachine* the, void* theStream,xsGetter theGetter, xsStringValue thePath, xsIntegerValue theLine)`

| |
| --- |
| `the` | 

> A machine

| |
| --- |
| `theStream` | 

> Where to get characters

| |
| --- |
| `theGetter` | 

> How to get characters

| |
| --- |
| `thePath` | 

> The current path of the stream

| |
| --- |
| `theLine` | 

> The current line of the stream

| |
| --- |
| Returns | 

> `true` if successful, `false` otherwise

The current path and line are used to report errors and warnings. Pass `NULL` and 0 if these parameters do not make sense in your application.

##### Example

<div class="ccode"></div>

	void xsMainExecute(xsMachine* theMachine, int argc, char* argv[])
	{
		FILE* aFile;

		aFile = fopen(argv[1], "r");
		if (aFile) {
			if (xsExecute(theMachine, aFile, fgetc, argv[1], 1)))
				fprintf(stderr, "### %s: OK\n", argv[1]);
			else 
				fprintf(stderr, "### Cannot execute %s\n", argv[1]);
			fclose(aFile);
		}
		else
			fprintf(stderr, "### Cannot open %s\n", argv[1]);
	}

### Grammars

This section describes the grammar-related macros of XS in C, summarized in Table 3. The first macro described, `xsLink`, links a grammar, and the remaining macros apply only if a grammar has been provided. 

**Table 3.** Grammar-Related Macros

<table class="normalTable">
  <tbody>
    <tr>
      <th scope="col">Macro</th>
      <th scope="col">Description</th>
    </tr>
    <tr>
      <td><code>xsLink</code></td>
      <td>Links a grammar dynamically</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsParse</code></p>
        <p><code>xsParseBuffer</code></p>
      </td>
      <td>Parses an XML document, transforming it into an ECMAScript instance</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsParse1 ... xsParse8</code></p>
        <p><code>xsParseBuffer1 ... xsParseBuffer8</code></p>
      </td>
      <td>Same as <code>xsParse</code> and <code>xsParseBuffer</code>, but with 1 to 8 parameters that are prototypes that constrain the kind of XML document accepted</td>
    </tr> 
    <tr>
      <td>
        <p><code>xsSerialize</code></p>
        <p><code>xsSerializeBuffer</code></p>
      </td>
      <td>Serializes an ECMAScript instance, transforming it into an XML document</td>
    </tr> 
    <tr>
      <td><code>xsScript</code></td>
      <td>Enables a callback to test whether it has been called by script code, directly or indirectly</td>
    </tr> 
    <tr>
      <td><code>xsSandbox</code></td>
      <td>Enables framework code to use properties created at runtime by script code</td>
    </tr> 
  </tbody>
</table>


#### xsLink

As described in the section [Machine Allocation](#machine-allocation), the `xsNewMachine` macro takes one grammar: the main grammar of the machine, which is the grammar of the application. To link grammars dynamically--for instance, grammars of plug-ins--use the `xsLink` macro.

`void xsLink(xsGrammar* theGrammar)`

| |
| --- |
| `theGrammar` | 

> The grammar to link

If the machine cannot link the grammar--for instance, if it conflicts with an already linked grammar--the `xsLink` macro throws an exception.

To use the `xsLink` macro, first load the library to be dynamically linked; then look for the grammar in the symbols of the library and pass it to `xsLink`.

##### Example

<div class="ccode"></div>

	void linkLibrary(xsMachine* the, char* thePath, char* theName)
	{
	#if mxWindows
		HINSTANCE anInstance;
		char aName[256];
		xsGrammar* aGrammar;

		anInstance = LoadLibrary(thePath);
		xsElseError(anInstance != NULL);
		strcpy(aName, theName);
		strcat(aName, "Grammar");
		aGrammar = (xsGrammar*)GetProcAddress(anInstance, aName);
		xsElseError(aGrammar != NULL);
		xsLink(aGrammar);
	#elif mxMacOSX
		const struct mach_header *anImage;
		char aName[256];
		NSSymbol aSymbol;
		xsGrammar* aGrammar;
		NSLinkEditErrors errors;
		int aNumber;
		const char* aFile;
		const char* aMessage;
		
		anImage = NSAddImage(thePath, NSADDIMAGE_OPTION_RETURN_ON_ERROR);
		if (anImage == NULL) goto error;   
		strcpy(aName, "_");
		strcat(aName, theName);
		strcat(aName, "Grammar");
		aSymbol = NSLookupSymbolInImage(anImage, aName, 
			NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW |
			NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR);
		if (aSymbol == NULL) goto error;
		aGrammar = NSAddressOfSymbol(aSymbol);
		if (aGrammar == NULL) goto error;
		xsLink(aGrammar);
		return;
	error:
		NSLinkEditError(&errors, &aNumber, &aFile, &aMessage);
		xsThrow(xsNew1(xsGlobal, xsID("Error"), xsString((char*)aMessage)));
	#else
		void* aHandle;
		char aName[256];
		xsGrammar* aGrammar;
		
		aHandle = dlopen(thePath, RTLD_NOW);
		if (aHandle == NULL) goto error;
		strcpy(aName, theName);
		strcat(aName, "Grammar");
		aGrammar = dlsym(aHandle, aName);
		if (aGrammar == NULL) goto error;
		xsLink(aGrammar);
		return;
	error:
		xsThrow(xsNew1(xsGlobal, xsID("Error"), xsString(dlerror())));
	#endif
	}

Alternatively, if the plug-in needs no C callback, `xsc` can build a binary file instead of a dynamically linked library. The binary file is atom-based. To use `xsLink` in this case, first load the binary file; then fill a grammar with the atoms and pass it to `xsLink`.

##### Example

<div class="ccode"></div>

	void linkBinary(xsMachine* the, char* thePath)
	{
	#define XS_ATOM_CONTAINER 0x58533131 /* 'XS11' */
	#define XS_ATOM_SYMBOLS 0x53594D42 /* 'SYMB' */
	#define XS_ATOM_CODE 0x434F4445 /* 'CODE' */
		typedef struct {
			long atomSize;
			unsigned long atomType;
		} Atom;
		FILE* aFile = NULL;
		size_t aSize;
		char* aBuffer = NULL;
		xsGrammar aGrammar;
		char* aPointer;
		Atom anAtom;

		xsTry {
			aFile = fopen(thePath, "rb");
			xsElseError(aFile != NULL);
			xsElseError(fseek(aFile, 0, SEEK_END) == 0);
			aSize = ftell(aFile);
			xsElseError(fseek(aFile, 0, SEEK_SET) == 0);
			aBuffer = malloc(aSize);
			xsElseError(aBuffer != NULL);
			xsElseError(fread(aBuffer, aSize, 1, aFile) == 1);
			aPointer = aBuffer;
			anAtom.atomSize = ntohl(((Atom*)aPointer)->atomSize);
			anAtom.atomType = ntohl(((Atom*)aPointer)->atomType);
			if (anAtom.atomType != XS_ATOM_CONTAINER)
				xsError(EINVAL);
			aPointer += sizeof(anAtom);
			anAtom.atomSize = ntohl(((Atom*)aPointer)->atomSize);
			anAtom.atomType = ntohl(((Atom*)aPointer)->atomType);
			if (anAtom.atomType != XS_ATOM_SYMBOLS)
				xsError(EINVAL);
			aGrammar.symbols = aPointer + sizeof(anAtom);
			aGrammar.symbolsSize = anAtom.atomSize - sizeof(anAtom);
			aPointer += anAtom.atomSize;
			anAtom.atomSize = ntohl(((Atom*)aPointer)->atomSize);
			anAtom.atomType = ntohl(((Atom*)aPointer)->atomType);
			if (anAtom.atomType != XS_ATOM_CODE)
				xsError(EINVAL);
			aGrammar.code = aPointer + sizeof(anAtom);
			aGrammar.codeSize = anAtom.atomSize - sizeof(anAtom);
			aGrammar.callback = NULL;
			aGrammar.name = NULL;
			xsLink(&aGrammar);
			free(aBuffer);
			aBuffer = NULL;
			fclose(aFile);
			aFile = NULL;
		}
		xsCatch {
			if (aBuffer != NULL)
				free(aBuffer);
			if (aFile != NULL)
				fclose(aFile);
		xsThrow(xsException);
		}
	}

#### xsParse and xsParseBuffer

If you provided a grammar with the `xsNewMachine` or `xsLink` macro (or both), you can parse XML documents with the `xsParse` or `xsParseBuffer` macro.  As described in the [*XS*](../xs) document, parsing converts an XML document to an ECMAScript instance; the instance corresponds to the root of the XML document and will typically include other instances attached to it as properties. In XS in C, the XML document is retrieved from either a stream (`xsParse`) or a buffer (`xsParseBuffer`).

The `xsParse` macro takes a stream and a getter for getting the characters that constitute the XML document. For example, if the XML document is a file, the stream can be declared as `FILE*` and the getter can be `fgetc` (although the getter can be anything else with equivalent behavior). 

<div class="ccode"></div>

	typedef int (*xsGetter)(void* theStream);

Both `xsParse` and `xsParseBuffer` take one or more of the following flags.

	enum {
		xsSourceFlag = 1,
		xsNoErrorFlag = 2,
		xsNoWarningFlag = 4,
		xsDebugFlag = 128
	};

	typedef unsigned char xsFlag;

`xsSlot xsParse(void* theStream, xsGetter theGetter, xsStringValue thePath, xsIntegerValue theLine, xsFlag theFlags)`

| |
| --- |
| `the` | 

> A machine

| |
| --- |
| `theStream` | 

> Where to get the characters constituting the XML document

| |
| --- |
| `theGetter` | 

> How to get the characters constituting the XML document

| |
| --- |
| `thePath` | 

> The current path of the stream

| |
| --- |
| `theLine` | 

> The current line of the stream

| |
| --- |
| `theFlags` | 

> A combination of flags

| |
| --- |
| Returns | 

> A reference to the resulting ECMAScript instance

The `xsParseBuffer` macro instead takes a buffer and a size.

`xsSlot xsParseBuffer(xsStringValue theBuffer, xsIntegerValue theSize, xsStringValue thePath, xsIntegerValue theLine, xsBooleanValue theFlags)`

| |
| --- |
| `the` | 

> A machine

| |
| --- |
| `theBuffer` | 

> The characters constituting the XML document

| |
| --- |
| `theSize` | 

> The number of characters

| |
| --- |
| `thePath` | 

> The current path of the stream

| |
| --- |
| `theLine` | 

> The current line of the stream

| |
| --- |
| `theFlags` | 

> A combination of flags

| |
| --- |
| Returns | 

> A reference to the resulting ECMAScript instance 

The current path and line are used to report errors and warnings. Pass `NULL` and 0 if these parameters do not make sense in your application.

Set `xsSourceFlag` in `theFlags` to keep the sources of the parsed functions. (As noted in the [*XS*](../xs) document, the source code can be accessed and assigned by tools created for editing objects, including functions.) Set `xsDebugFlag` to see the sources of the parsed functions in `xsbug`.

`xsNoErrorFlag` and `xsNoWarningFlag` determine the action the parser will take (or not) if it detects any of the following problems; errors or warnings that would otherwise be reported are not reported if the corresponding flag is set.

- Unknown namespaces, elements, attributes, and processing instructions (PIs)

- Redundant elements and attributes

- Missing namespaces in elements, attributes, and PIs
 
##### Example

<div class="ccode"></div>

	xsResult = xsParse(aFile, fgetc, aPath, aLine,
		xsSourceFlag | xsNoErrorFlag | xsNoWarningFlag);

If the XML document cannot be parsed by the grammar (or grammars), or if the machine has no grammar, the `xsParse` and `xsParseBuffer` macros throw an exception.

The `xsParse*` and `xsParseBuffer*` macros (where `*` is `1` to `8`) are variants of `xsParse` and `xsParseBuffer`. The additional parameters are prototypes that constrain the kind of XML document accepted by the parser. Such prototypes must have a root pattern or `xslib` signals an error.

#### xsSerialize and xsSerializeBuffer

If you provided a grammar with the `xsNewMachine` or `xsLink` macro (or both), you can serialize XML documents with the `xsSerialize` or `xsSerializeBuffer` macro. As described in the [*XS*](../xs) document, serializing converts an ECMAScript instance to an XML document; the instance corresponds to the root of the XML document and will typically include other instances attached to it as properties. In XS in C, the XML document is written to either a stream (`xsParse`) or a buffer (`xsParseBuffer`).

The `xsSerialize` macro takes a stream and a putter for writing the strings that constitute the XML document.

<div class="ccode"></div>

	typedef int (*xsPutter)(xsStringValue theString, void* theStream);`

 For example, if the XML document is a file, the stream can be declared as `FILE*` and the putter can be `fputs` (although the putter can be anything else with equivalent behavior).
 
 `void xsSerialize(xsSlot theRoot, void* theStream, xsPutter thePutter)`

| |
| --- |
| `theRoot` | 

> A reference to the ECMAScript instance

| |
| --- |
| `theStream` | 

> Where to put the strings constituting the XML document

| |
| --- |
| `thePutter` | 

> How to put the strings constituting the XML document

| |
| --- |
| `theSize` | 

> The number of characters

The `xsSerializeBuffer` macro instead takes a buffer and a size and returns the number of characters generated by the serializer. The buffer can be `NULL` and the size can be 0 to measure the XML document; the macro will run the serializer and only return the size of the data, without generating any output. You could then allocate a block of memory of that size and call the macro again, passing the size.

`xsIntegerValue xsSerializeBuffer(xsSlot theRoot, void* theBuffer, xsIntegerValue theSize)`
      
| |
| --- |
| `theRoot` | 

> A reference to the ECMAScript instance

| |
| --- |
| `theBuffer` | 

> The characters constituting the XML document

| |
| --- |
| `theSize` | 

> The number of characters

| |
| --- |
| Returns | 

> The number of characters generated by the serializer

If the ECMAScript instance cannot be serialized by the grammar (or grammars), or if the machine has no grammar, the xsSerialize and `xsSerializeBuffer` macros throw an exception.

#### xsScript and xsSandbox

If you provided grammars with the `xsNewMachine` or `xsLink` macro (or both), `xslib` can execute code either outside the sandbox (framework code) or inside the sandbox (script code); see the [*XS*](../xs) document for details.

With the `xsScript` macro, a callback can test at runtime whether it has been called by script code (directly or indirectly). This macro is similar to the `script` property of the global object `xs` in ECMAScript.

`xsIntegerValue xsScript()`

| |
| --- |
| Returns | 

> The depth of the script code call, or 0 if no script call

The result of the test is a number: 0 means called by framework code, 1 means called directly by script code, and greater than 1 means called indirectly by script code.

In C, framework code can use properties created at runtime by script code with the `xsSandbox` macro; see the [*XS*](../xs) document for details. It is similar to the `Object.prototype.sandbox` property in ECMAScript.

`xsSlot xsSandbox(xsSlot theThis)`

| |
| --- |
| `theThis` | 

> A reference to an instance

| |
| --- |
| Returns | 

> A handle for using runtime properties created by script code

The handle can be cached in a variable like any slot.

#####In ECMAScript:

	this.sandbox.foo
	delete this.sandbox.foo
	this.sandbox.foo()
	new this.sandbox.foo()

#####In C:

<div class="ccode"></div>

	 xsGet(xsSandbox(xsThis), xsID("foo"));
	 xsDelete(xsSandbox(xsThis), xsID("foo"));
	 xsCall0(xsSandbox(xsThis), xsID("foo"));
	 xsNew0(xsSandbox(xsThis), xsID("foo"));

<a id="glossary"></a>
## Glossary

##### constructor

In ECMAScript, a function that has a `prototype` property and that the `new` operator invokes to build an instance. The value of the `prototype` property becomes the prototype of the instances that the constructor builds.

##### context

A pointer to an area where you can store and retrieve information for the virtual machine of `xslib` in your callbacks.

##### direct slot

One of the slot types that correspond to the ECMAScript primitive types (undefined, null, boolean, number, and string), plus an integer slot provided as an optimization.

##### ECMAScript

An object-oriented, prototype-based language for implementing application logic and control.

##### framework

In XS, a set of ECMAScript prototypes built from a grammar, thereby defining the global scope of the application.

##### grammar

In XS, an XML document (with a `.xs` extension) with ECMAScript embedded in it. The grammar defines ECMAScript objects and their properties (including functions).

##### host

In ECMAScript terminology, an application that uses `xslib`.

##### host constructor

In XS, a constructor whose implementation is in C rather than ECMAScript.

##### host function

In XS, a function whose implementation is in C rather than ECMAScript.

##### host object

In XS, an object with data that can be directly accessed only in C.

##### indirect slot

A type of slot that contains a reference to an instance of an object, function, array, and so on; corresponds to the ECMAScript `reference` type.

##### instance

An object that inherits properties from another object, which is called its *prototype*.

##### parser

In XS, an entity that is built from a grammar to transform an XML document into ECMAScript instances.

##### pattern

An optional attribute of an XS element that identifies an element or attribute in an XML document. The result is a template that provides a mapping between ECMAScript and XML.

##### PI

Processing instruction; a type of node in an XML document.

##### property

In ECMAScript, a value accessed by name within an object (in contrast to items accessed by index within an array); in XS in C, a slot accessed by index within an object (just as an item is accessed by index within an array).

##### prototype

An object from which another object (called an instance) inherits properties.

##### sandbox

An environment that is restricted to prevent untrusted code from harming the device on which the code is running. The sandbox for XS application scripts includes the standard features defined in the ECMAScript specification plus additional features as defined and permitted by the XS grammar.

##### serializer

In XS, an entity that is built from a grammar to transform ECMAScript instances into an XML document.

##### slot

An opaque structure in which everything in `xslib` is stored, and which is manipulated only through XS in C.

##### XML

Extensible Markup Language; a markup language, designed for web documents, that enables the creation of customized tags. An XML document is a tree of nodes consisting of elements, attributes, text, and processing instructions (PIs).

##### XS

A toolkit, consisting of a runtime library and a command-line tool, that is designed for developing standards-based, networked, interactive multimedia applications (GUI-based runtimes) or command-line tools for various devices. See also [**xslib**](#xslib) and [**xsc**](#xsc).

##### XS in C

The C interface of `xslib`.

##### xsbug

The XS debugger, used to debug grammars, their scripts, and patterns.

<a name="xsc"></a>
##### xsc

The command-line tool part of XS. It compiles grammars into XS bytecode, which is executed by the XS virtual machine that is contained within `xslib`.

<a name="xslib"></a>
##### xslib

The runtime library part of XS.

