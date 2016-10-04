<!-- Version: 160412-KO / Last reviewed: December 2015

KinomaJS has several ways to specify color and transparency, which are based on the variety of methods available in CSS. This Tech Note provides details about each supported method, including examples and things to watch out for.
-->

<img alt="" src="img/specifying-color-in-kinomajs_icon.png" class="technoteIllus" >

#Specifying Color in KinomaJS

**Mike Jennings, QA Program Manager**  
April 1, 2015

KinomaJS has several ways to specify color and transparency, which are based on the variety of methods available in CSS. This Tech Note provides details about each supported method, including examples and things to watch out for.

As stated throughout this Tech Note, specifying color in KinomaJS is modeled after CSS3. In the following examples, we will define a `style` with a `color` value of purple. The  examples are shown in JavaScript (with a shorthand version, if available) and in the optional XML markup form.


##Basic Color Keywords

First are the CSS3 [basic color keywords](http://www.w3.org/TR/css3-color/#html4): `aqua`, `black`, `blue`, `fuchsia`, `gray`, `green`, `lime`, `maroon`, `navy`, `olive`, `purple`, `red`, `silver`, `teal`, `white`, and `yellow`. The `orange` keyword included in [CSS2's list of basic color keywords](http://www.w3.org/TR/CSS2/syndata.html#value-def-color) was not supported in early releases of KinomaJS but has since been added.

#####JavaScript:

```
var purpleStyle = new Style({color:"purple"});
var purpleStyle = new Style("purple");
```

#####XML:

```xml
<style id="purpleStyle" color="purple">
```
	
The keyword `transparent` is also supported, and is in fact the default used when colors are unspecified or invalid.

>**Note:** The [extended color keywords](http://www.w3.org/TR/css3-color/#svg-color) are not currently supported.

##Hexadecimal Notation

[Hexadecimal notation](http://www.w3.org/TR/css3-color/#numerical) is the traditional web color specification, preceded by `#` and with each successive pair of hexadecimal digits representing the 0-255 values for red, green, and blue, respectively.
	
#####JavaScript:

```
var purpleStyle = new Style({color:"#FF00FF"});
var purpleStyle = new Style("#FF00FF");
```

#####XML:

```xml
<style id="purpleStyle" font="24px Arial" color="#FF00FF">
```

###Transparency Using Hex Notation

Unique to KinomaJS, there is a method for specifying opacity (or alpha) in hex notation, in ARGB format: simply prepend the traditional RGB format with another hex pair that specifies the level of opacity, from 0 to 255. In the following example, `7F` sets the opacity to semitransparent (127 decimal, or 50%).

#####JavaScript:

```
var transparentPurpleStyle = new Style({color:"#7FFF00FF"});
var transparentPurpleStyle = new Style("#7FFF00FF");
```

#####XML:

```xml
<style id="transparentPurpleStyle" font="24px Arial" color="#7FFF00FF">
```

##Functional Notation

Colors can also be specified using the CSS3 `rgb`, `rgba`, or `hsl` functional notation.

###RGB

You can specify color as an [`rgb`](http://www.w3.org/TR/css3-color/#rgb-color) function with integers between 0 and 255 for red, green, and blue.

#####JavaScript:

```
var purpleStyle = new Style({color:"rgb(50%,0%,50%)"});
var purpleStyle = new Style("rgb(255,0,255)");
```

#####XML:

```xml
<style id="purpleStyle" font="24px Arial" color="rgb(255,0,255)">
```
	
You can even specify percentages, so you do not need to write a conversion function.

#####JavaScript:

```
var purpleStyle = new Style({font:"24px Arial",color:"rgb(50%,0%,50%)");
```

#####XML:

```xml
<style id="purpleStyle" font="24px Arial" color="rgb(50%,0%,50%)">
```

###RGBA

You can add alpha-channel transparency using [`rgba`](http://www.w3.org/TR/css3-color/#rgba-color) functional notation. The alpha must be a value between 0 and 1 and will be rounded to two decimal places.

#####JavaScript:

```
var transparentPurpleStyle = new Style({color:"rgba(255,0,255,0.5)"});
var transparentPurpleStyle = new Style("rgba(255,0,255,0.5)");
```

#####XML:

```xml
<style id="transparentPurpleStyle" font="24px Arial" color="rgba(255,0,255,0.5)">
```
	
###HSL

Finally, you can use the CSS3 [`hsl`](http://www.w3.org/TR/css3-color/#hsl-color) functional notation. The first value, the hue angle, is in degrees: 0 to 360 inclusive. The second and third values are saturation and lightness, respectively, and are expressed as percentages. Like `rgba`, `hsl` supports the alpha channel, using the `hsla` function; the transparency value must be between 0 and 1 and will be rounded to two decimal places.

#####JavaScript:

```
var purpleStyle = new Style({color:"hsl(270,100%,50%)"});
var transparentPurpleStyle = new Style({color:"hsla(270,100%,50%,0.66)"});

var purpleStyle = new Style("hsl(270,100%,50%)");
var transparentPurpleStyle = new Style("hsla(270,100%,50%,0.66)");
```

#####XML:

```xml
<style id="purpleStyle" font="24px Arial" color="hsl(270,100%,50%)">
<style id="transparentPurpleStyle" font="24px Arial" color="hsla(270,100%,50%,0.66)">
```

##Pitfalls

These CSS color specifications are to be passed as *strings*, and the runtime parses them as such. If you are changing them programmatically, be sure to build them as strings.

```
var myShade;
for (var i; i<256; i=i+5) {
	myShade = "rgb(" + i + "," + i + "," + i + ")";
	myButton.color=myShade;
}
```
	
The following code *will not work*. While it is valid JavaScript code, it is not a string, so it will not be properly recognized, and the result will be transparent.

```
myShade = rgb(i,i,i)	// This will NOT work
```
	
Also, KinomaJS is currently not as forgiving about whitespace in specifiers as web browsers are. Tabs, for example, cannot always be used to line up long lists of color specifications in RGB notation; thay may be considered invalid and return `transparent`.