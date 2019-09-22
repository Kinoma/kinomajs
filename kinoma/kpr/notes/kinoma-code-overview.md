<!-- Version: 160908-CR / Primary authors: Chris Krueger and Mike Jennings / Last reviewed: August 2016 by Mike

Kinoma Code is an IDE that integrates a JavaScript editor and debugger with several features specific to Kinoma devices. Kinoma Code itself is mostly developed in JavaScript and is based on KinomaJS.
-->

# Kinoma Code Overview

<!--UI issue #110351 logged to change orange DOCUMENTS tile to FILES.-->

[Kinoma Code](http://kinoma.com/develop/code/) is an IDE that integrates a JavaScript editor and debugger with several features specific to Kinoma devices. Kinoma Code itself is mostly developed in JavaScript and is based on KinomaJS. 

>**Note:** This document covers Kinoma Code version 1.0b5.

## Kinoma Code Window
To introduce the terminology, Figure 1 shows the Kinoma Code window.

**Figure 1.** Kinoma Code Window
![Kinoma Code Layout](img/code-layout.jpg)

### Sidebar 

<!--UI issue #110352 logged re renaming View menu to Activities after Enter Full Screen is moved into a Windows menu.-->

The Kinoma Code *sidebar* enables selection of any of six activities: Devices, Projects, Debugger, Samples, Documentation, and Net Scanner. (You can also select an activity using the **View** menu and its handy keyboard shortcuts.) As shown in Figure 2, a notch in the sidebar indicates the currently selected activity. Activities can run in the background; for instance, the Devices and Debugger activities run even when not selected.

**Figure 2.** Kinoma Code Sidebar

![Kinoma Code Side Bar](img/sidebar.jpg) 

Badges and bounces are used in the sidebar to report events: the Devices icon bounces when devices appear and disappear on the current network; the Projects icon shows a badge with the number of files that have been edited but not saved; and the Debugger icon bounces when a project breaks. 

### Tiles Area

Each activity uses the *tiles area* (to the right of the sidebar) to display its data in several tiles (Figure 3).

* The Devices activity uses tiles similar to the application-type tiles on Kinoma Create. The active tile has a corner folded down. 

* All other activities use tiles that have headers; you can click the header to expand a tile (listing the items within it) or to collapse an expanded tile. The Projects activity uses directory-type tiles (with an arrowhead toggle on the left for expanding/collapsing), and the Documentation activity's tiles similarly expand to show the document titles and tables of contents.

**Figure 3.** Tiles Area (Devices, Projects, and Documentation)
![Kinoma Code Tiles Area](img/tiles.jpg)

### View Area

Selecting a tile--or an item in a tile, depending on the activity--opens it in the *view area*  (to the right of the tiles area). Some kinds of views are specific to activities; for instance, the Projects and Debugger activities use the *code view* when a file is selected, as shown in Figure 4. Other views are specific to tiles; Figure 4 shows the view that the Devices activity uses when a Pin Explorer tile is selected.

**Figure 4.** Code View (Projects/Debugger) and Tile-Specific View (Pin Explorer in Devices)
![Kinoma Code View Area](img/views.jpg)

You can dismiss an item from the view area by clicking the **x** in the right corner of its header.

### Console
All activities can use the Kinoma Code *console* (Figure 5) to display console output from logs, traces, and other instrumentation. The console can be minimized, cleared, or pinned to the right side instead of the bottom of the Kinoma Code window.

**Figure 5.** Console
![Kinoma Code Console](img/console.jpg)

You can attach a Kinoma Element device via USB to get the console connected to it. (See the CLI reference section in the [*Programmer's Guide to Kinoma Element*](../element/) for details on the Kinoma Element command-line interface.)

## Kinoma Code Activities
The section provides more information about each of the six Kinoma Code activities. 

### Devices
As shown in Figure 6, the gray bar at the top of the tiles area for the Devices activity (as well as the Projects, Debugger, and Samples activities) is a drop-down list of the available target devices (Figure 6), including:

* Hardware running KinomaJS on the current network. (If a device is removed from the network, it disappears from the devices list.)
 
* Built-in simulators for Kinoma Create and Kinoma Element. To make a simulator an active target, you need to launch it (by clicking **Launch**).

**Figure 6.** Devices List  
![Kinoma Code Device List](img/device-dropdown.jpg)

Tiles are displayed for the currently selected device. Depending on the kind of device, several tiles are built in for configuring the network, changing the settings (using the Settings app), muxing the pins, and using Pin Explorer to interact with sensors. Figure 7 shows Kinoma Create selected in the devices list and (on the right) what the view area displays for the resulting tiles. 

<a id="device-tiles"></a>
**Figure 7.** Device Tiles (Kinoma Create)
![Kinoma Code Device Tiles](img/device-tiles.jpg)

For Kinoma Create, tiles showing an application name or icon in white (like the `lab-test` tile in Figure 7) are project applications that are compatible with the target device; click them to build, transfer, and debug those projects on the device. 

- If you click the project application name or icon, the application is transferred via Wi-Fi and launched but not installed. 

- If you click **Install**, the project application is installed and can be launched from the home screen on the Kinoma Create device. Once the application is installed, the button changes to **Delete**, which uninstalls the application from the device.

Figure 8 shows the different icons that indicate whether a device is a network device, a simulator you have launched, or a Kinoma Element device connected via USB for console access.

**Figure 8.** Device Icons

![Kinoma Code CLI](img/console-access.jpg)

### Projects

Projects in Kinoma Code are represented by directories, displayed in directory-type tiles in the tiles area. You can drag and drop directories into Kinoma Code. If the directory contains a `project.json` file (or legacy `application.xml` file), Kinoma Code considers it a project and allows it to be run on appropriate devices. Editing and saving the JSON file modifies the project.

New projects can be created easily, with the **File** > **New Project** command. The **New Project** command displays a dialog in which you select a target device and a template and enter a title for the project (see Figure 9).

**Figure 9.** New Project Dialog

![Kinoma Code New Project Dialog](img/new-project.jpg)

You can open an existing project with the **File** > **Open Project** command. Opening a project imports it into the tiles area as a new directory. (It does not relocate the files on disk.) It will also import directories of files that are not projects.

There is also a **File** > **Open File** command for opening a file directly into the view area, without importing the file into a project. If the file type is not supported in Kinoma Code, the view area displays a "No viewer found!" error message.

Figure 10 illustrates working with project files via the tiles area and the view area.

* For each project, a green tile appears that expands to list the files in that project; selecting a file (like `main.js` in the `lab-test` project here) opens it in code view in the view area. Figure 10 shows the different controls that may appear in the project tile to enable you to run the project, add files to it, or delete it.

* The orange DOCUMENTS tile lists the currently edited files; files are automatically added here when you begin to modify them, and you can return to a file by selecting it in this list.  Files with unsaved changes will display a bullet at the left.

* A gray search tile lets you search across all projects. Enter the search text (and optionally set related controls), and the search tile will immediately expand, displaying the results list directly below the search tile. It will include every file that contains that text, expanding to show every line where it was found--in the example shown, two lines in just one file. Selecting a search result will display the file in the view area and scroll to the selected occurrence, as shown in Figure 10. The search tile can be collapsed using the triangle at the left, preserving the current search results; if the triangle is clicked again or the search criteria are changed, the tile expands.

* In the view area, controls in a file's title bar enable you to jump to a particular variable, function, class, and so on (via a menu) or close the file. 

**Figure 10.** Working with Project Files
![Kinoma Code Documents](img/documents.jpg)

To search within only the file you are currently working in, use one of the **Find** (or **Replace**) commands in the **Edit** menu. The file's title bar will display a search field and related controls, and the search results will be highlighted in the view area.

**Figure 11.** Searching Within an Individual File

![Kinoma Code Documents](img/document-search.jpg)

### Debugger
The Debugger activity in Kinoma Code provides a full-featured debugger for KinomaJS. You can concurrently debug several projects running on different devices. 

As shown in Figure 12, controls are available for stepping through code. If multiple virtual machines are currently running, you can select the one you want from a drop-down list to the right of the step controls. 

**Figure 12.** Kinoma Code Debugger

![Kinoma Code Debugger](img/debugger.jpg)

The Debugger activity includes the following tiles: 

- BREAKPOINTS displays breakpoints for all files in all projects. Clicking one of them opens the corresponding file in the view area and scrolls to that breakpoint. Hovering over the right end of the BREAKPOINTS tile header reveals a **Clear All** button that will clear all breakpoints from all files in all projects (which cannot be undone).

- CALLS displays the call stack for the currently selected virtual machine.  Clicking a call in this list displays the local variables from that call's scope in the LOCALS tile.

- LOCALS displays the state of the local variables in the currently selected scope.

- MODULES displays the modules in use at the breakpoint. Clicking one of them expands to display relevant information.

- GLOBALS displays the current state of the machine globals.

### Samples

You can browse and run any of more than 100 sample application projects on hardware or simulators. These samples are from the [Kinoma open source repository](https://github.com/Kinoma/KPR-examples) on GitHub and are updated often. The **File** > **New Project** command lets you use any sample as the starting point for a new application.

Initially there is a gray tile for each sample project, expanded to display a description of the project. When you hover over one of these tiles, a control appears that lets you download the sample code so you can edit/run it on local devices. Once the code is downloaded, the gray tile is replaced by a green directory-type tile, with controls that enable you to run the sample, see its description again, or remove the code from the local drive. Figure 13 shows four sample projects, with the first and third downloaded; the first project's `main.js` file is selected and so is displayed in the view area.

**Figure 13.** Samples Activity
![Kinoma Code Samples](img/code-samples.jpg)

Sample projects can be filtered by category, using the pop-up list that appears when you click in the orange SAMPLES tile.

**Figure 14.** Clicking in SAMPLES Tile to Filter Samples

![Kinoma Code Sample Filter](img/sample-filter.jpg)

### Documentation

The Documentation activity gathers together the technical documentation for building applications for Kinoma Element and Kinoma Create. 

Controls in a document's title bar enable you to jump to a particular part of the document (via a table of contents menu) or close the document. You can search within a document as you can within a file in the Projects activity: the **Edit** > **Find** command displays a search field and related controls in the document's title bar.  

**Figure 15.** Documentation Activity
![Kinoma Code Documentation](img/code-documentation.jpg)

### Net Scanner
The Net Scanner activity is a tool for discovering network services available on the local network. This activity is helpful for debugging connected applications by verifying that a device is on the network at the expected IP address and with the expected services available. You can use it to discover mDNS or Zeroconf services, view mDNS/Zeroconf details (name, type, host, IP address, and port), discover SSDP services, and view SSDP XML service descriptions. The discovered mDNS/Zeroconf services are categorized and sorted by either IP address or type, toggled using the **Sort by** switch in the gray NET SCANNER bar (see Figure 16).

**Figure 16.** Net Scanner Sorting Options
![Kinoma Code Netscanner](img/code-netscanner.jpg)

## Preferences
Selecting **Preferences** from the **Kinoma Code** menu displays (in the view area) the screen shown in Figure 17. The four categories of preferences--DEVICES, PROJECTS, DEBUGGER, and SAMPLES--expand to show one or more items; hover over any item to see a brief description of it (and possibly additional controls). Details about the preferences are provided in the sections that follow.  

**Figure 17.** Kinoma Code Preferences

![Kinoma Code Preferences](img/prefs-default-state.png)

### DEVICES
As shown in Figure 17, the DEVICES preference category expands into the items described below, all related to the devices list in the Devices, Projects, Debugger, and Samples activities. A switch next to each of these items (which similarly expand) indicates the state of the items within: switched to the left indicates that all the items within are off, to the right indicates that all the items are on, and in the middle indicates mixed states among the items. Operating this switch affects all the items within.

#### Discovery
The Discovery preferences control what devices will appear in the devices list when they are found on the local network (see Figure 18).

**Figure 18.** Discovery Preferences under DEVICES

![Kinoma Code Preferences](img/devices-discovery.png)

For a device to appear in the devices list, it must be switched on (to the right) in these preferences, must be connected to the same local network as the computer running Kinoma Code, and must be running a KinomaJS "debug shell" application. Kinoma-branded devices run a debug shell at startup by default. Other devices in the devices list can run the "embed shell" debug shell to wirelessly run and debug KinomaJS applications using Kinoma Code. The embed shell is available as part of the KinomaJS open source release on GitHub.

#### Serial Console
The Serial Console preferences (Figure 19) enable or disable serial console access to devices connected to the computer via USB. Switching a device off (to the left) will prevent its serial connection from appearing in the devices list.

**Figure 19.** Serial Console Preferences under DEVICES

![Kinoma Code Preferences](img/devices-serialconsole.png)

#### Simulators

The Simulators preferences (Figure 20) control what device simulators appear in the devices list when they are found on the local host or the local network and also let you integrate custom-built simulators with Kinoma Code. Switching a device off (to the left) will prevent its serial connection from appearing in the devices list. 

**Figure 20.** Simulators Preferences under DEVICES

![Kinoma Code Preferences](img/devices-simulators.png)

Each preference displays the name of the simulator that will be launched when you click **Launch** for that simulator in the devices list. Hovering over an item will reveal buttons that enable you to either use the default simulator bundled in Kinoma Code or locate a custom-built simulator elsewhere (see Figure 21).

**Figure 21.** Hovering to Reveal Buttons for a Simulator

![Kinoma Code Preferences](img/devices-simulators-locate.png)

#### Beta Program
The Beta Program preferences (Figure 22) allow you to update Kinoma devices with beta (pre-release) firmware. Switching these preferences on (to the right) is generally not recommended; enable them only if there is a specific feature in development that you have agreed to test. Pre-release builds are rarely announced on the Kinoma Forums, do not always come with release notes, and never come with any guarantees of reliability or stability.

**Figure 22.** Beta Program Preferences under DEVICES

![Kinoma Code Preferences](img/devices-betaprogram.png)

Turning the Beta Program preference on enables access to the latest beta version of the Kinoma Create or Kinoma Element device firmware. Whenever the firmware on a device is not the latest version--that is, the latest beta or release version, depending on whether this preference is set or not--an **UPDATE** notification appears in the Settings app for that device (see [Figure 7](#device-tiles)); hovering there causes an **Apply** button to appear, which you can click to apply the update. If you are running beta firmware and want to revert to the latest release version, turn the Beta Program preference off and the **UPDATE** button will then appear in the Settings app.

### PROJECTS

<!--UI issue #110353 logged to change Folder to Directory in all preferences.-->

The PROJECTS preference category expands to a preference that lets you customize how Kinoma Code deals with new projects. The Projects Folder preference (Figure 23) displays the current directory in which new projects will be created and lets you select a different directory for that purpose. 

**Figure 23.** Projects Folder Preference under PROJECTS

![Kinoma Code Preferences](img/projects-default.png)

Hovering over the Projects Folder item will reveal a **Locate** button that enables you to select a different directory (Figure 24).

**Figure 24.** Hovering to Reveal Locate Button for Projects Folder

![Kinoma Code Preferences](img/projects-default-locate.png)

If there are already KinomaJS projects in the new directory, they will appear in the projects list displayed by the Projects activity.

### DEBUGGER
The DEBUGGER preference category expands to two items (Figure 25) that enable you to select a directory containing the KinomaJS open source release and the preferred port to use for debugging wirelessly. Hovering over the first item, KinomaJS Folder, will reveal a **Locate** button that lets you select a directory containing the KinomaJS open source release. The Kinoma Code debugger will then be able to display source code from KinomaJS when KinomaJS applications cause a break in core code.

**Figure 25.** Hovering to Reveal Locate Button for KinomaJS Folder

![Kinoma Code Preferences](img/debugger-kinomajs-unset.png)

Use the **Locate** button to select the root directory of the KinomaJS open source release repository downloaded or cloned from GitHub. The preference will show the directory path (see Figure 26).

**Figure 26.** KinomaJS Folder Preference Showing Selected Root Directory Path

![Kinoma Code Preferences](img/debugger-kinomajs-set.png)

The Port Number item shows the default port to be used for debugging wirelessly; if that port conflicts with something on the local network, you can choose a different port number here (enter the new number and then click the **Set** button that appears).

### SAMPLES
The SAMPLES item expands to a preference that determines where applications downloaded using the Samples activity will be located. The Samples Folder preference (Figure 27) displays the current directory where sample applications should be downloaded. 

**Figure 27.** Samples Folder Preference under SAMPLES

![Kinoma Code Preferences](img/samples-default.png)

Hovering over the Samples Folder item will reveal a **Locate** button that enables you to select a different directory.

**Figure 28.** Hovering to Reveal Locate Button for Samples Folder

![Kinoma Code Preferences](img/samples-locate.png)

Samples already downloaded will not be moved; to remain in the list of samples displayed by the Samples activity when Kinoma Code is relaunched, they will need to be manually moved to the new location.
