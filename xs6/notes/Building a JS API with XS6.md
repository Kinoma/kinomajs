# Building a JS API with XS6

KinomaJS is a cross-platform runtime environment for delivering applications on consumer electronic products. At the core of KinomaJS, there is XS6, a library that implements a JS virtual machine conforming to the 6th edition of the ECMAScript specifications.

You can also use XS6 alone to make your existing device scriptable by the end-user. This tech note will get you started.

## Getting XS6

Get KinomaJS from GitHub. Here is the part of the tree to look at: 
	
	kinomajs
		xs6
			doc
			includes
			makefiles
				linux
				mac
				win
			sources
				fsk
				mc
				tool
			tools
			xsbug
	
* The `doc` directory contains **XS in C**, which documents the C programming interface of XS6.
* The `includes` directory contains `xs.h`, which defines the C programming interface of XS6.
* The `makefiles ` directory contains the make files, organized by platform. 
* The `sources` directory contains the sources themselves and the host directories. 
* The `tools` directory contains the sources of several tools used by KinomaJS. 
* The `xsbug ` directory contains the sources of the XS6 debugger, **xsbug**. 

## Building XS6

Let us start by building **xslib**, the library that you will add to your project.

Define the `XS6` environment variable to be the path to the XS6 directory. Then

	cd $XS6/makefiles/linux
	make debug -f xslib.make
	
That will build both static and dynamic libraries, use the kind that fits your project and add it to your make file. 

You can also build **xsbug** to debug your scripts. The debugger works remotely so you can build it on the desktop platform of your choice: Linux GTK, Mac OS or Windows.

	cd $XS6/makefiles/linux
	make debug -f xsbug.make
	
To connect your device to **xsbug**, set the `XSBUG_HOST` environment variable to the IP address of the desktop platform that is running **xsbug**.

## Customizing XS6

By default **xslib** use the tool host and platform defined in `$XS6/sources/tool`. It is a generic command-line host that works on Linux, Mac OS and Windows. Like the rest of XS6, the tool host relies on the POSIX programming interface. 

But you can completely customize your host. Duplicate the `$XS6/sources/tool` directory and modify some of the files it contains:

* `xs6Host.c`: This file implements essential services for the virtual machine: loading modules and programs from archive, binary or text files, queuing and performing promise jobs, etc.
* `xs6Platform.h`: XS6 uses the POSIX programming interface thru macros (for instance `c_malloc` instead of `malloc`). Use this file to declare alternative system functions.
* `xs6Platform.c`: And use this file to implement them. 

Then duplicate `XS6/makefiles/linux/xslib.make` and modify it to use your host directory. You can of course keep your custom host private but if you want to publish it, Kinoma welcomes your contribution. 

## Executing a script

Here is a simple C tool that will execute a script with XS6. 

	/* test.c */
	#include <xs.h>
	int main(int argc, char* argv[]) 
	{
		xsCreation creation = {
			128 * 1024 * 1024, 	/* initial chunk size */
			16 * 1024 * 1024, 	/* incremental chunk size */
			8 * 1024 * 1024, 	/* initial heap slot count */
			1 * 1024 * 1024,	/* incremental heap slot count */
			4 * 1024, 			/* stack slot count */
			12 * 1024, 			/* key slot count */
			1993, 				/* name modulo */
			127 				/* symbol modulo */
		};
		xsMachine* machine = xsCreateMachine(&creation, NULL, "my virtual machine", NULL);
		xsBeginHost(machine);
		xsRunProgram(argv(1));
		xsEndHost(machine);
		xsDeleteMachine(machine);
		return 0;
	}
	
Here is a simple JS script.

	/* test.js */
	trace("Hello, world!\n");
	
Build the tool then

	./test test.js
	
will trace `Hello, world!` in **xsbug**.

## Step by step	
	
Firstly you create a virtual machine with `xsCreateMachine`.

		xsCreation creation = {
			128 * 1024 * 1024, 	/* initial chunk size */
			16 * 1024 * 1024, 	/* incremental chunk size */
			8 * 1024 * 1024, 	/* initial heap slot count */
			1 * 1024 * 1024,	/* incremental heap slot count */
			4 * 1024, 			/* stack slot count */
			12 * 1024, 			/* key slot count */
			1993, 				/* name modulo */
			127 				/* symbol modulo */
		};
		xsMachine* machine = xsCreateMachine(&creation, NULL, "test", NULL);
	
* The first parameter is a structure with various members to control how the machine will manage memory. XS6 uses chunks for arrays, buffers, byte codes, strings, and slots for everything else. The garbage collector marks slots and chunks then reclaims slots and sweeps chunks.   
* The second parameter is the archive, a memory mapped read-only file that XS6 can use to execute code in place. See **XS in C** for details. Pass `NULL` to use XS6 without an archive.
* The third parameter is the name of your virtual machine. **xsbug** will display that name.
* The fourth parameter is a context that you can use to store and retrieve information in your callbacks. Pass `NULL` to use no context.

Then you enter the virtual machine with `xsBeginHost` to setup its stack and its exception chain.

		xsBeginHost(machine);

Then you call xsRunProgram to execute the script itself. 

		xsRunProgram(argv(1));
				
Then you leave the virtual machine with `xsEndHost` to cleanup its stack and its exception chain.

		xsEndHost(machine);

Eventually, you delete the virtual machine with `xsDeleteMachine`.

		xsDeleteMachine(machine);
	
## Building your host

Between `xsBeginHost` and `xsRunProgram` you can define host functions for scripts to use.

Here is for instance a host function that returns the value of an environment variable:

	void xs_getenv(xsMachine* the)
	{
		xsStringValue result = getenv(xsToString(xsArg(0)));
		if (result)
			xsResult = xsString(result);
	}
 
All host functions take one parameter, the virtual machine, which has to be named `the`. The C programming interface of XS6 provides a lot of macros to convert between C and JS types, to get and set properties, to call functions, etc. Here the `xs_getenv` function uses `xsToString` and `xsString` to convert C strings from and to JS strings. See **XS in C** for details.

To make such a function available to scripts, use `xsNewHostFunction` and set a global.

	xsResult = xsNewHostFunction(xs_getenv, 1);
	xsSet(xsGlobal, xsID("getenv"), xsResult);
	
The first parameter of `xsNewHostFunction` is the C function, the second parameter is the expected number of arguments.  `xsNewHostFunction` returns an instance of `Function` that uses the C function instead of byte code. The `xsID` macro creates a property name. 

Now you can change your script to trace environment variables.

	/* test.js */
	trace(getenv("XS6") + "\n");
	trace(getenv("XSBUG_HOST") + "\n");
	
## Runtime Model

Creating and deleting a virtual machine to execute a script is of course a trivial runtime model. Most devices use a runtime loop to trigger events, read and write sockets, schedule timers, etc. 

Usually you will create the virtual machine at launch and delete the virtual machine at exit. You can then load scripts at will. Scripts can follow the programming interface you defined to handle events that the runtime dispatches, like scripts in web pages.  

For instance, let us imagine that, when some value changed, your device calls a C function, `c_onValueChanged`, with a name and a value.

	extern void c_onValueChanged(Device* device, char* name, double value);

You want to provide the same feature to scripts so they can do something like:

	/* test.js */
	device.onValueChanged = function(name, value) {
		trace("onValueChanged " + name + " " + value + "\n");
	}
	
You will need a reference to the virtual machine, it can be a global or it can be in your device structure.

	device->machine = xsCreateMachine(&creation, NULL, "test", NULL);
	
And you will need to create the `device` global object for scripts to use.

	xsBeginHost(device->machine);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	xsSet(xsGlobal, xsID("device"), xsResult);
	xsEndHost(device->machine);
		
C code always need to enter and leave properly the virtual machine in order to use the C programming interface of XS6. Here`xsNewInstanceOf(xsObjectPrototype)` is creating a JS object, it is equivalent to `{} ` in JS.
	
Then you can implement `c_onValueChanged` to call the script.

	void c_onValueChanged(Device* device, char* name, double value)
	{
		xsBeginHost(device->machine);
		xsResult = xsGet(xsGlobal, xsID("device"));
		xsCall2(xsResult, xsID("onValueChanged"), xsString(name), xsNumber(value));
		xsEndHost(device->machine);
	}
		
The `c_onValueChanged` function uses `xsCall2` to call the property of an object with two arguments.
		
## Arbitrary Callbacks

Arbitrary callbacks are also a commonly used pattern in runtime models. 

For instance, let us imagine that your programming interface provides a C function, `c_setTimeout`, to schedule another C function with a delay and an opaque argument:

	extern void c_setTimeout((*callback)(void* context), double delay, void* argument);

You want to provide the same feature to scripts so they can do something like:
	
	/* test.js */
	setTimeout(function(it) { trace(it + "\n"); }, 1000, "WOW!");
	
Firstly you need to define the host function that scripts will call.

	typedef struct xs_setTimeout_data {
		xsMachine* machine;
		xsSlot function;
		xsSlot argument;
	};

	static void xs_setTimeout_callback(void* it);
	
	xs_setTimeout(xsMachine* the) {
		xs_setTimeout_data* data = malloc(sizeof(xs_data));
		data->machine = the;
		data->function = xsArg(0);
		xsRemember(data->function);
		data->reference = xsArg(2);
		xsRemember(data->reference);
		c_setTimeout(xs_setTimeout_callback, xsToNumber(xsArg(1)), data);
	}
	
The `xs_setTimeout` function allocates a C structure to store the JS function to callback and its argument. Notice the calls to `xsRemember`. They are necessary because you are storing JS slots outside of the virtual machine and the garbage collector needs to know where its roots are. Then the `xs_setTimeout` function calls the corresponding C function.

When the delay expired, the C callback is called. 

	void xs_setTimeout_callback(void* it)
	{
		xs_setTimeout_data* data = it;
		xsBeginHost(data->machine);
		xsCallFunction1(xsAccess(data->function), xsUndefined, xsAccess(data->argument))
		xsForget(data->argument);
		xsForget(data->function);
		xsEndHost(data->machine);
		free(data);
	}

As usual, the C callback needs to enter and leave properly the virtual machine. Besides that it can use all macros of **XS in C**. Here it uses `xsCallFunction1` to call the stored function with the stored argument and `xsForget` to tell the garbage collector that the slots are no roots anymore. Then it frees the C structure.


