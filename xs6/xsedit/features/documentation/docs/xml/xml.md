<!-- Version: 160812-CR / Primary author: Patrick Soquet / Last reviewed: ? Not since CR added comments/queries in it starting in March 2014

KinomaJS supports an XML document format with embedded JavaScript; some developers find this format more convenient. This document provides details on the elements that make up the KinomaJS XML format.
-->

#KinomaJS XML Reference

##About This Document

KinomaJS supports an XML document format with embedded JavaScript; some developers find this format more convenient. This document provides details on the elements that make up KinomaJS XML format.

>**Note:** Use of the KinomaJS XML format is optional; the same results can be achieved using only JavaScript. (In fact, Kinoma Code converts KinomaJS XML to JavaScript source code to run on Kinoma Create.) 

The companion document [*KinomaJS Overview*](../overview/) introduces the KinomaJS XML format along with much more information that this document assumes you are already familiar with (including a Glossary of terms used in this document).
 
##KinomaJS XML Element Reference

<!--From CR: The wording shown parenthetically in the next paragraph is per Patrick; however, based on old overview material (now in the Overview doc's glossary), templates do not instantiate but rather are instantiated. Ideally this usage would be consistent. (Ditto in all similar places)-->

This section provides details on the elements that make up the optional KinomaJS XML format. For each element, a brief description is provided. (For elements described as defining constructors or templates that create or instantiate objects, more information about the objects can be found in the 
[*KinomaJS JavaScript Reference*](../javascript/) document.) Then the following information is presented if relevant. 

####Tag

This element's XML tag. The namespace is always `http://www.kinoma.com/kpr/1`. 

####Attributes

This element's XML attributes. Attributes have one of the types listed in Table 1. Attributes that are required are indicated as such in this section. Unless noted otherwise, the default values for optional attributes are as shown in the table. 

**Table 1.** XML Attribute Types

<table class="normalTable">
    <tr>
      <th scope="col">Type</th>
      <th scope="col">Description</th>
      <th scope="col">Examples</th>
      <th scope="col">Default (unless<br>noted otherwise)</th>
	</tr>
   
    <tr>
	  <td><code>boolean</code></td>
	  <td>A boolean value</td>
      <td>Must be <code>true </code> or <code>false</code></td>
      <td><code>false</code></td>
    </tr>

    <tr>
      <td><code>expression</code></td>
      <td>A JavaScript expression</td>
      <td><code>true</code><br/><code>25</code><br/><code>'Santa Claus'</code><br/><code>$.party</code></td>
      <td><code>undefined</code></td>
    </tr>

    <tr>
      <td><code>identifier</code></td>
      <td>A JavaScript identifier</td>
      <td><code>Present</code></td>
      <td><code>undefined</code></td>
    </tr>

    <tr>
      <td><code>number</code></td>
      <td>A number</td>
      <td><code>12</code></td>
      <td><code>0</code></td>
    </tr>

    <tr>
      <td><code>reference</code></td>
      <td>An identifier to reference an element in the same module or program <br/><br/>
      or<br/><br/> 
      The identifier of a <code>require</code> element followed by a dot and an identifier to reference an element in a required module</td>
      <td><code>Present</code><br/><br/>
      or<br/><br/>
      <code>CHRISTMAS.Present</code><br/></td>
      <td><code>null</code></td>
    </tr>

    <tr>
    </tr>
    
    <tr>
      <td><code>string</code></td>
      <td>A sequence of characters</td>
      <td><code>This is Christmas</code></td>
      <td>The empty string</td>
    </tr>
  </table>
    
####Elements

The XML elements within this element 

####CDATA

The characters within this element. Several elements hold JavaScript code, which is usually escaped like this:  

```
<![CDATA[ 
// JavaScript code
]]>
``` 

###Behavior Element

The behavior element defines a `Behavior` constructor and its prototype. Applications and shells use `Behavior` constructors to create `behavior` objects. 

####Tag

| | | |
| --- | --- | --- |
| `behavior` |

####Attributes

| | | |
| --- | --- | --- |
| `id` | `identifier` | required * | 

> The identifier of this `Behavior` constructor.

> &#42; A `behavior` element nested in a template cannot have an `id` attribute.

| | | |
| --- | --- | --- |
| `like` | `reference` | |

> A reference to a `Behavior` constructor (`Behavior` by default). The properties of the prototype of the referenced constructor are inherited by the prototype of the defined constructor.

####Elements
| | | |
| --- | --- | --- |
| `field` | 0 or more | |
| `method` | 0 or more | |
| `getter` | 0 or more | |
| `setter` | 0 or more | |

> The properties of the prototype of this `Behavior` constructor

###Block Element

The `block` element creates a paragraph inside a `text` element. 

####Tag

| | | |
| --- | --- | --- |
| `block` |

####Attributes
 
| | | |
| --- | --- | --- |
| `style` |` reference` | |

> The block's style, a reference to a `style` object. KinomaJS cascades the style of the `text` element.

####Elements
  
| | | |
| --- | --- | --- |
| `span` | 0 or more | |
| `wrap` | 0 or more | |

> The runs and contents to render

###Borders Element

The `borders` element sets the borders of a `skin` object.  

####Tag  
| | | |
| --- | --- | --- |
| `borders` |
  
####Attributes  
| | | |
| --- | --- | --- |
| `color` | `string` | |   

> The color of the borders, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`transparent` by default)

| | | |
| --- | --- | --- |
| `left` | `number` | |
| `right` | `number` | |
| `top` | `number` | | 
| `bottom` | `number` | |

> The size of the left, right, top, and bottom borders

####Elements  
| | | |
| --- | --- | --- |
| `states` | 0 or more | |   
 
> The border's states

### Canvas Element
The `canvas` element defines or extends a template to instantiate a `canvas` object.  

####Tag  
| | | |
| --- | --- | --- |
| `canvas` |

####Attributes  

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)). The `like` attribute, if any, must reference another `canvas` element. 

####Elements  

Same as for the `content` element (see [Elements](#content-element-elements) in the section [Content Element](#content-element))     

###Class Element

The `class` element defines a constructor and its prototype.   

####Tag 
 
| | | |
| --- | --- | --- |
| `class` |

####Attributes

| | | |
| --- | --- | --- |
| `id` | `identifier` | required | 
  
> The identifier of the constructor

| | | |
| --- | --- | --- |
| `like` | `reference` | |

<!-- From CR: Is "a constructor" correct below? Compare to description of `like` under for behavior and transition elements (e.g., "A reference to a `Behavior` constructor (`Behavior` by default)")-->

> A reference to a constructor (`Object` by default). The properties of the prototype of the referenced constructor are inherited by the prototype of the defined constructor. 

####Elements  

| | | |
| --- | --- | --- |
| `constructor` | 0 or 1 | |   

> The parameters and body of the constructor. The default constructor has no parameters and an empty body.

| | | |
| --- | --- | --- |
| `field` | 0 or more | |     
| `getter` | 0 or more | |    
| `method` | 0 or more | |    
| `setter` | 0 or more | |   

> The properties of the prototype of the constructor

###Colorize Element

The `colorize` element configures an `effect` object to colorize the image.   

####Tag

| | | |
| --- | --- | --- |
| `colorize` |

####Attributes  

| | | |
| --- | --- | --- |
| `color` | `string` | |    

> The colorization color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`gray` by default)

| | | |
| --- | --- | --- |
| `opacity` | `number` | |    

> The colorization opacity, as a number between 0 and 1 (the default)

###Column Element

The `column` element defines or extends a template to instantiate a `column` object.    

####Tag  

| | | |
| --- | --- | --- |
| `column` |   

####Attributes  

Same as for the `container` element (see [Attributes](#container-element-attributes) in the section [Container Element](#container-element)). The `like` attribute, if any, must reference another `column` element. 

####Elements  

Same as for the `container` element (see [Elements](#container-element-elements) in the section [Container Element](#container-element)) 

###Constructor Element

The `constructor` element defines the parameters and the body of the constructor defined by a `class` element.   

####Tag 
 
| | | |
| --- | --- | --- |
| `constructor` | 

####Attributes

<!--From CR re the following: In all cases where I changed how the number of identifiers is presented, I made an assumption re the minimum number; please check all.-->
 
| | | |
| --- | --- | --- |
| `params` | `identifier` (0 or more, separated by commas) |

> The parameters of the constructor

####CDATA  

JavaScript code, the body of the constructor 

<a id="container-element"></a>  
### Container Element

The `container` element defines or extends a template to instantiate a `container` object.   

####Tag  

| | | |
| --- | --- | --- |
| `container` |

<a id="container-element-attributes"></a>

####Attributes

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)), plus the following. The `like` attribute, if any, must reference another `container` element.
   
| | | |
| --- | --- | --- |
| `clip` | `boolean` |     
> If `true`, the container clips its contents to its bounds.

<a id="container-element-elements"></a>

####Elements

Same as for the `content` element (see [Elements](#content-element-elements) in the section [Content Element](#content-element)), plus:   

| | | |
| --- | --- | --- |
| `canvas` | 0 or more | |     
| `column` | 0 or more | |     
| `container` | 0 or more | |     
| `content` | 0 or more | |     
| `iterate` | 0 or more | |     
| `label` | 0 or more | |     
| `layout` | 0 or more | |     
| `line` | 0 or more | |     
| `picture` | 0 or more | |     
| `media` | 0 or more | |     
| `scope` | 0 or more | |     
| `scroller` | 0 or more | |     
| `select` | 0 or more | |     
| `text` | 0 or more | |     
| `thumbnail` | 0 or more | |
    
> The sequence of templates to instantiate and instructions to execute to build the contents of the `container` object

<a id="content-element"></a>
### Content Element

The `content` element defines or extends a template to instantiate a `content` object.   

####Tag

| | | |
| --- | --- | --- |
| `content` |

<a id="content-element-attributes"></a>
####Attributes

>**Note:** When instantiating, KinomaJS evaluates the defined `active`, `state`, `variant`, and `visible` attributes to set the corresponding properties of the `content` object, and the defined ` left`, `width`, `right`, `top`, `height`, and `bottom` attributes to set the `coordinates` property of the `content` object.

| | | |
| --- | --- | --- |
| `active` | `expression` | |    
 
> If `true`, the behavior of the content gets touch and key events.

| | | |
| --- | --- | --- |
| `anchor` | `identifier` | |    
 
> The anchor of the `content` object. When instantiating, KinomaJS assigns a property to the instantiating data: the identifier of the property is the anchor, and the value of property is the `content` object.

| | | |
| --- | --- | --- |
| `behavior` | `reference` | |    
 
> A reference to a `Behavior` constructor. When instantiating, KinomaJS calls the referenced constructor to create a `behavior` object that becomes the behavior of the `content` object.

| | | |
| --- | --- | --- |
| `bottom` | `expression` |    
 
> The `bottom` coordinate of the `content` object

| | | |
| --- | --- | --- |
| `height` | `expression` |    

> The height of the `content` object

| | | |
| --- | --- | --- |
| `id` | `identifier` | required * |    
 
> The identifier of the template

> &#42; A `content` element nested in a template cannot have an `id` attribute.

| | | |
| --- | --- | --- |
| `left` | `expression` |    

> The `left` coordinate of the `content` object

| | | |
| --- | --- | --- |
| `like` | `reference` |    
 
> A reference to a `content` element

| | | |
| --- | --- | --- |
| `name` | `identifier` |    
 
> The name of the `content` object

| | | |
| --- | --- | --- |
| `right` | `expression` |    

> The `right` coordinate of the `content` object

| | | |
| --- | --- | --- |
| `skin` | `reference` | |     

> The skin of the `content` object, a reference to a `skin` object   
 
| | | |
| --- | --- | --- |
| `state` | `expression` | |    
 
> The initial state of the `content` object 

| | | |
| --- | --- | --- |
| `style` | `reference` | |    

> The style of the `content` object, a reference to a `style` object. KinomaJS cascades styles across the containment hierarchy.  

| | | |
| --- | --- | --- |
| `top` | `expression` |    

> The `top` coordinate of the `content` object

| | | |
| --- | --- | --- |
| `variant` | `expression` | |    

> The initial variant of the `content` object

| | | |
| --- | --- | --- |
| `visible` | `expression` | |    
 
> If `true` (the default), the `content` object is displayed.

| | | |
| --- | --- | --- |
| `width` | `expression` | |    

> The width of the `content` object


<a id="content-element-elements"></a>
####Elements
  
| | | |
| --- | --- | --- |
| `behavior` | 0 or 1 | | 
 
> A `Behavior` constructor. When instantiating, KinomaJS calls the `Behavior` constructor, if any, to create the `behavior` object that becomes the behavior of the `content` object. The `content` element cannot have both a `behavior` attribute and a `behavior` element.

| | | |
| --- | --- | --- |
| `skin` | 0 or 1 | | 

> The skin of the `content` object, a `skin` object. The `content` element cannot have both a `skin` attribute and a `skin` element.

| | | |
| --- | --- | --- |
| `style` | 0 or 1 | | 
 
> The style of the `content` object, a `style` object. The `content` element cannot have both a `style` attribute and a `style` element.

###Effect Element

The `effect` element creates an `effect` object.   

####Tag 
 
| | | |
| --- | --- | --- |
| `effect` |   

####Attributes
  
| | | |
| --- | --- | --- |
| `id` | `identifier` | required |  
 
> The identifier of the `effect` object
 
####Elements 
 
| | | |
| --- | --- | --- |
| `colorize` | 0 or 1 | |
| `inner-glow` | 0 or 1 | |     
| `inner-hilite` | 0 or 1 | |    
| `inner-shadow` | 0 or 1 | |     
| `mask` | 0 or 1 | |     
| `outer-glow` | 0 or 1 | |     
| `outer-hilite` | 0 or 1 | |     
| `outer-shadow` | 0 or 1 | |     
| `shade` | 0 or 1 | |    
 
> The configuration of the `effect` object

###Field Element

The `field` element defines a property in the prototype of the constructor defined by a `class`, `behavior`, or `transition` element.   

####Tag  

| | | |
| --- | --- | --- |
| `field` |

####Attributes  

| | | |
| --- | --- | --- |
| `id` | `identifier` | required |    
 
> The identifier of the property
 
| | | |
| --- | --- | --- |
| `value` | `expression` | |     
 
> The initial value of the property 

###Function Element

The `function` element defines a function in a program or a module.   

####Tag  

| | | |
| --- | --- | --- |
| `function` |

####Attributes  

| | | |
| --- | --- | --- |
| `id` | `identifier` | required |

> The identifier of the function
   
| | | |
| --- | --- | --- |
| `params` | `identifier` (0 or more, separated by commas) |    

> The parameters of the function 

####CDATA 
 
JavaScript code, the body of the function 

###Getter Element

The `getter` element defines a property getter in the prototype of the constructor defined by a `class`, `behavior`, or `transition` element.   

####Tag   

| | | |
| --- | --- | --- |
| `getter` |

####Attributes  

| | | |
| --- | --- | --- |
| `id` | `identifier` | required |    
 
> The identifier of the property

####CDATA  

JavaScript code, the body of the getter 

###Handler Element

The `handler` element creates and inserts a `handler` object in the set of active handlers of the application.   

####Tag  

| | | |
| --- | --- | --- |
| `handler` |
  
####Attributes

| | | |
| --- | --- | --- |
| `behavior` | `reference` | |     

> A reference to a `Behavior` constructor. KinomaJS calls the referenced constructor, if any, to create the `behavior` object that becomes this handler's behavior.
 
| | | |
| --- | --- | --- |
| `path` | `string` | required |    
 
> The path of the messages that invoke this handler

####Elements  

| | | |
| --- | --- | --- |
| `behavior` | 0 or 1 | | 
 
> A `Behavior` constructor, which KinomaJS calls to create the `behavior` object that becomes this handler's behavior. The `handler` element cannot have both a `behavior` attribute and a `behavior` element.

###Include Element

The `include` element executes a script as a program. The script can be a KinomaJS XML document with a `program` root, JavaScript source, or byte codes.  

####Tag 

| | | |
| --- | --- | --- |
| `include` |
  
####Attributes 
 
| | | |
| --- | --- | --- |
| `path` | `string` | required |    
 
> The path to the script to execute as a program

###Inner-Glow Element

The `inner-glow` element configures an `effect` object to apply a glow to the inner boundary of the image.

####Tag

| | | |
| --- | --- | --- |
| `inner-glow` |
  
####Attributes 
 
| | | |
| --- | --- | --- |
| `blur` | `number` | |    
 
> The glow softness (1 by default)
 
| | | |
| --- | --- | --- |
| `color` | `string` | |    
 
> The glow color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`white` by default)
 
| | | |
| --- | --- | --- |
| `opacity number` | | |        
 
> The glow opacity, as a number between 0 and 1 (the default)
 
| | | |
| --- | --- | --- |
| `radius` | `number` | |    
 
> The glow radius inward from the boundary, in pixels (1 by default)

###Inner-Hilite Element

The `inner-hilite` element configures an `effect` object to apply a highlight to the inner boundary of the image.

####Tag  
 
| | | |
| --- | --- | --- |
| `inner-hilite` |

####Attributes

| | | |
| --- | --- | --- |
| `blur` | `number` | |    
 
> The highlight softness (2 by default)
 
| | | |
| --- | --- | --- |
| `color` | `string` | |    
 
> The highlight color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`white` by default)
 
| | | |
| --- | --- | --- |
| `opacity` | `number` | |   
 
> The highlight opacity, as a number between 0 and 1 (the default)
 
| | | |
| --- | --- | --- |
| `x, y` | `number` | |    
 
> The highlight offsets relative to the boundary, in pixels (2 by default)

###Inner-Shadow Element

The `inner-shadow` element configures an `effect` object to apply a shadow to the inner boundary of the image.

####Tag

| | | |
| --- | --- | --- |
| `inner-shadow` |
  
####Attributes

| | | |
| --- | --- | --- |
| `blur` | `number` | |    
 
> The shadow softness (2 by default)   
 
| | | |
| --- | --- | --- |
| `color` | `string` | |    
 
> The shadow color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`black` by default)   
 
| | | |
| --- | --- | --- |
| `opacity` | `number` | |     
 
> The shadow opacity, as a number between 0 and 1 (the default)
 
| | | |
| --- | --- | --- |
| `x, y` | `number` | |    

> The shadow offsets relative to the boundary, in pixels (2 by default)

###Iterate Element

The `iterate` element is an instruction that enables instantiating templates and executing instructions for each item in an array.   

####Tag   

| | | |
| --- | --- | --- |
| `iterate` |

####Attributes 

| | | |
| --- | --- | --- |
| `on` | `expression` | required |    
 
> The array to iterate on. In its turn, each item becomes the instantiating data.

####Elements  

| | | |
| --- | --- | --- |
| `canvas` | 0 or more | |     
| `column` | 0 or more | |     
| `container` | 0 or more | |     
| `content` | 0 or more | |     
| `iterate` | 0 or more | |     
| `label` | 0 or more | |     
| `layout` | 0 or more | |     
| `line` | 0 or more | |     
| `picture` | 0 or more | |     
| `media` | 0 or more | |     
| `scope` | 0 or more | |     
| `scroller` | 0 or more | |     
| `select` | 0 or more | |     
| `text` | 0 or more | |     
| `thumbnail` | 0 or more | |    

> The sequence of templates to instantiate and instructions to execute for each item in the array    

###Label Element

The `label` element defines or extends a template to instantiate a `label` object.

####Tag   

| | | |
| --- | --- | --- |
| `label` | 

####Attributes  

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)), plus the following. The `like` attribute, if any, must reference another `label` element.   
 
| | | |
| --- | --- | --- |
| `editable` | ` boolean` | |       
| `hidden` | ` boolean` | |       
| `selectable` | ` boolean` | |      
 
> If `true`, the string is editable, hidden, or selectable, respectively.
 
| | | |
| --- | --- | --- |
| `string` | `expression` | |    
 
> The string to render. When instantiating, KinomaJS evaluates the defined `string` attribute to set the corresponding properties of the `label` object.


###Layout Element

The `layout` element defines or extends a template to instantiate a `layout` object.    

####Tag  

| | | |
| --- | --- | --- |
| `layout` |   

####Attributes  

Same as for the  `container` element (see [Attributes](#container-element-attributes) in the section [Container Element](#container-element)). The `like` attribute, if any, must reference another `layout` element. 

####Elements  

Same as for the `container` element (see [Elements](#container-element-elements) in the section [Container Element](#container-element))     

###Line Element

The `line` element defines or extends a template to instantiate a `line` object.    

####Tag  

| | | |
| --- | --- | --- |
| `line` |    

####Attributes  

Same as for the `container` element (see [Attributes](#container-element-attributes) in the section [Container Element](#container-element)). The `like` attribute, if any, must reference another `line` element. 

####Elements  

Same as for the `container` element (see [Elements](#container-element-elements) in the section [Container Element](#container-element)) 

###Margins Element

The `margins` element sets the margins of a `skin` or `style` object.    

####Tag  

| | | |
| --- | --- | --- |
| `margins` |
  
####Attributes  

| | | |
| --- | --- | --- |
| `left` | `number` | |     
| `right` | `number` | |     
| `top` | `number` | |     
| `bottom` | `number` | |    
 
> The size of the left, right, top, and bottom margins

###Mask Element

The `mask` element configures an `effect` object to mask the image.   

####Tag  

| | | |
| --- | --- | --- |
| `mask` |

####Attributes  

| | | |
| --- | --- | --- |
| `texture` | `reference` | |     

> A reference to the `texture` object to use as a mask    

###Media Element

The `media` element defines or extends a template to instantiate a `media` object.   

####Tag  

| | | |
| --- | --- | --- |
| `media` |   

####Attributes  

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)), plus the following. The `like` attribute, if any, must reference another `media` element.   
 
| | | |
| --- | --- | --- |
| `aspect` | ` string` | |    
 
> How the audio's album art or the video is displayed, as `draw` (the default), `fill`, `fit`, or `stretch`

| | | |
| --- | --- | --- |
| `mime` | `expression` | |    
 
> The MIME type of the audio or video
 
| | | |
| --- | --- | --- |
| `url` | `expression` | |   
 
> The URL of the audio or video. Inside a program, the value of this attribute is merged with the URL of the application. Inside a module, the value of this attribute is merged with the URL of the module.

###Method Element

The `method` element defines a function property in the prototype of the constructor defined by a `class`, `behavior`, or `transition` element.  

####Tag  

| | | |
| --- | --- | --- |
| `method` |

####Attributes  
| | | |
| --- | --- | --- |
| `id` | `identifier` | required |    
 
> The identifier of the function
 
| | | |
| --- | --- | --- |
| `params` | `identifier` (0 or more, separated by commas) |    
 
> The parameters of the function

####CDATA  

JavaScript code, the body of the function

###Module Element

The `module` element is the root element of an XML document that generates a JavaScript module.   

####Tag  

| | | |
| --- | --- | --- |
| `module` |

####Elements  

| | | |
| --- | --- | --- |
| `behavior` | 0 or more | |     
| `class` | 0 or more | |     
| `function` | 0 or more | |     
| `transition` | 0 or more | |     
| `variable` | 0 or more | |    
 
> The constructors, functions, and variables defined by the module
 
| | | |
| --- | --- | --- |
| `canvas` | 0 or more | |     
| `column` | 0 or more | |     
| `container` | 0 or more | |     
| `content` | 0 or more | |     
| `label` | 0 or more | |     
| `layout` | 0 or more | |     
| `line` | 0 or more | |     
| `picture` | 0 or more | |     
| `port` | 0 or more | |     
| `media` | 0 or more | |     
| `scroller` | 0 or more | |     
| `text` | 0 or more | |     
| `texture` | 0 or more | |     
| `thumbnail` | 0 or more | |   
 
> The templates defined by the module
 
| | | |
| --- | --- | --- |
| `effect` | 0 or more | |     
| `handler` | 0 or more | |     
| `skin` | 0 or more | |     
| `style` | 0 or more | |     
| `texture` | 0 or more | |    
 
> The objects created by the module  
 
| | | |
| --- | --- | --- |
| `private` | 0 or more | |    
 
> The constructors, functions, objects, templates, and variables available only to the module itself
 
| | | |
| --- | --- | --- |
| `require` | 0 or more | |         
 
> Executes a script as a module and creates an object with the constructors, functions, objects, and variables exported by the module  
 
| | | |
| --- | --- | --- |
| `script` | 0 or more | |    
 
> JavaScript code

###Outer-Glow Element

The `outer-glow` element configures an `effect` object to apply a glow to the outer boundary of the image.

####Tag  

| | | |
| --- | --- | --- |
| `outer-glow` |

####Attributes  

| | | |
| --- | --- | --- |
| `blur` | `number` | | 
   
> The glow softness (1 by default)
 
| | | |
| --- | --- | --- |
| `color` | `string` | |  
 
> The glow color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`white` by default)   
 
| | | |
| --- | --- | --- |
| `opacity` | `number` | |
 
> The glow opacity, as a number between 0 and 1 (the default)   
 
| | | |
| --- | --- | --- |
| `radius` | `number` | | 

> The glow radius inward from the boundary, in pixels (1 by default) 

###Outer-Hilite Element

The `outer-hilite` element configures an `effect` object to apply a highlight to the outer boundary of the image.

####Tag  

| | | |
| --- | --- | --- |
| `outer-hilite` |
  
####Attributes  

| | | |
| --- | --- | --- |
| `blur` | `number` | | 
 
> The highlight softness (2 by default)   
 
| | | |
| --- | --- | --- |
| `color` | `string` | | 
 
> The highlight color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`white` by default)   
 
| | | |
| --- | --- | --- |
| `opacity` | `number` | |  
 
> The highlight opacity, as a number between 0 and 1 (the default)
 
| | | |
| --- | --- | --- |
| `x, y` | `number` | |    
 
> The highlight offsets relative to the boundary, in pixels (2 by default)

###Outer-Shadow Element

The `outer-shadow` element configures an `effect` object to apply a shadow to the outer boundary of the image.

####Tag  

| | | |
| --- | --- | --- |
| `outer-shadow` |

####Attributes  

| | | |
| --- | --- | --- |
| `blur` | `number` | | 
 
> The shadow softness (2 by default)   
 
| | | |
| --- | --- | --- |
| `color` | `string` | | 
 
> The shadow color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (  `black` by default)
 
| | | |
| --- | --- | --- |
| `opacity` | `number` | |
   
> The shadow opacity, as a number between 0 and 1 (the default)
 
| | | |
| --- | --- | --- |
| `x, y` | `number` | | 
   
> The shadow offsets relative to the boundary, in pixels (2 by default)      

###Picture Element

The `picture` element defines or extends a template to instantiate a `picture` object.  

####Tag  

| | | |
| --- | --- | --- |
| `picture` |   

####Attributes  

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)), plus the following. The `like` attribute, if any, must reference another `picture` element.   
 
| | | |
| --- | --- | --- |
| `aspect` | ` string` | |       
 
> How the picture is displayed
 
| | | |
| --- | --- | --- |
| `mime` | `expression` | |    
 
> The MIME type of the image. When instantiating, KinomaJS evaluates the defined `mime` attribute to set the corresponding properties of the `picture` object.
 
| | | |
| --- | --- | --- |
| `url` | `expression` | |       
 
> The URL of the image. Inside a program, the value of this attribute is merged with the URL of the application. Inside a module, the value of this attribute is merged with the URL of the module. When instantiating, KinomaJS evaluates the defined `url` attribute to set the corresponding properties of the `picture` object.

####Elements  

Same as for the `content` element (see [Elements](#content-element-elements) in the section [Content Element](#content-element))  

###Port Element

The `port` element defines or extends a template to instantiate a `port` object.   

####Tag  
| | | |
| --- | --- | --- |
| `port` |  

####Attributes  

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)). The `like` attribute, if any, must reference another `port` element. 

####Elements  

Same as for the `content` element (see [Elements](#content-element-elements) in the section [Content Element](#content-element)) 

###Private Element

By default, a module exports all the constructors, functions, objects, templates, and variables it defines to modules and programs requiring it. The `private` element enables defining constructors, functions, objects, templates, and variables available only to the module itself.   

####Tag  

| | | |
| --- | --- | --- |
| `private` |

####Elements 

| | | |
| --- | --- | --- |
| `behavior` | 0 or more | |     
| `class` | 0 or more | |     
| `function` | 0 or more | |     
| `transition` | 0 or more | |     
| `variable` | 0 or more | |    
 
> The private constructors, functions, and variables defined by the module
 
| | | |
| --- | --- | --- |
| `canvas` | 0 or more | |     
| `column` | 0 or more | |     
| `container` | 0 or more | |     
| `content` | 0 or more | |     
| `label` | 0 or more | |     
| `layout` | 0 or more | |     
| `line` | 0 or more | |     
| `picture` | 0 or more | |     
| `port` | 0 or more | |      
| `media` | 0 or more | |     
| `scroller` | 0 or more | |     
| `text` | 0 or more | |     
| `texture` | 0 or more | |     
| `thumbnail`| 0 or more | |    
 
> The private templates defined by the module   
 
| | | |
| --- | --- | --- |
| `effect` | 0 or more | |     
| `handler` | 0 or more | |     
| `skin` | 0 or more | |     
| `style` | 0 or more | |     
| `texture` | 0 or more | |    
 
> The private objects created by the module

###Program Element

The `program` element is the root element of an XML document that generates an JavaScript program.   

####Tag  

| | | |
| --- | --- | --- |
| `program` | 

####Elements  

| | | |
| --- | --- | --- |
| `behavior` | 0 or more | |     
| `class` | 0 or more | |     
| `function` | 0 or more | |     
| `transition` | 0 or more | |     
| `variable` | 0 or more | |    
 
> The constructors, functions, and variables defined by the program
 
| | | |
| --- | --- | --- |
| `canvas` | 0 or more | |     
| `column` | 0 or more | |     
| `container` | 0 or more | |     
| `content` | 0 or more | |     
| `label` | 0 or more | |     
| `layout` | 0 or more | |     
| `line` | 0 or more | |     
| `picture` | 0 or more | |     
| `port` | 0 or more | |     
| `media` | 0 or more | |     
| `scroller` | 0 or more | |     
| `text` | 0 or more | |     
| `texture` | 0 or more | |     
| `thumbnail`| 0 or more | |    
 
> The templates defined by the program
 
| | | |
| --- | --- | --- |
| `effect` | 0 or more | |     
| `handler` | 0 or more | |     
| `skin` | 0 or more | |     
| `style` | 0 or more | |     
| `texture` | 0 or more | |    
 
> The objects created by the program
 
| | | |
| --- | --- | --- |
| `include` | 0 or more | |    
 
> Executes a script as a program
 
| | | |
| --- | --- | --- |
| `require` | 0 or more | |    
 
> Executes a script as a module and creates an object with the constructors, functions, objects, and variables exported by the module   
 
| | | |
| --- | --- | --- |
| `script` | 0 or more | |    
 
> JavaScript code

###Require Element

The `require` element executes a script as a module and creates an object with the constructors, functions, objects, and variables exported by the module. The script can be a KinomaJS XML document with a `module` root, JavaScript source, or byte codes. For more information, see the [Common JS Modules specification](http://wiki.commonjs.org/wiki/Modules/1.1).   

####Tag  

| | | |
| --- | --- | --- |
| `require` |

####Attributes  

| | | |
| --- | --- | --- |
| `id` | `identifier` | required |    
 
> The identifier of the created object
 
| | | |
| --- | --- | --- |
| `path` | `string` | required |    
 
> The path to the script to execute as a module

###Scope Element

The `scope` element is an instruction that enables focusing the instantiating data for a sequence of templates and instructions.   

####Tag  

| | | |
| --- | --- | --- |
| `scope` |

####Attributes  

| | | |
| --- | --- | --- |
| `with` | `expression` | required |    
 
> The value of the expression becomes the instantiating data.

####Elements  

| | | |
| --- | --- | --- |
| `canvas` | 0 or more | |     
| `column` | 0 or more | |     
| `container`| 0 or more | |     
| `content` | 0 or more | |     
| `iterate` | 0 or more | |     
| `label` | 0 or more | |     
| `layout` | 0 or more | |     
| `line` | 0 or more | |     
| `picture` | 0 or more | |     
| `media` | 0 or more | |     
| `scope` | 0 or more | |     
| `scroller` | 0 or more | |     
| `select` | 0 or more | |     
| `text` | 0 or more | |     
| `thumbnail`| 0 or more | |    
 
> The sequence of templates to instantiate and instructions to execute

###Script Element

The `script` element inserts JavaScript code in a module or a program.   

####Tag  

| | | |
| --- | --- | --- |
| `script` | 

####CDATA  

JavaScript code     

###Scroller Element

The `scroller` element defines or extends a template to instantiate a `scroller` object.   

####Tag  

| | | |
| --- | --- | --- |
| `scroller` |   

####Attributes  

Same as for the `container` element (see [Attributes](#container-element-attributes) in the section [Container Element](#container-element)), plus the following. The `like` attribute, if any, must reference another `scroller` element.   
 
| | | |
| --- | --- | --- |
| `loop` | ` boolean` | |    
 
> If `true`, the scroller loops its first content.

####Elements  

Same as for the `container` element (see [Elements](#container-element-elements) in the section [Container Element](#container-element)) 

###Select Element

The `select` element is an instruction that enables instantiating templates and executing instructions conditionally.   

####Tag  

| | | |
| --- | --- | --- |
| `select` |
  
####Attributes  

| | | |
| --- | --- | --- |
| `on` | `expression` | required |    
 
> The condition to test

####Elements  

| | | |
| --- | --- | --- |
| `canvas` | 0 or more | |     
| `column` | 0 or more | |     
| `container` | 0 or more | |     
| `content` | 0 or more | |     
| `iterate` | 0 or more | |     
| `label` | 0 or more | |     
| `layout` | 0 or more | |     
| `line` | 0 or more | |     
| `picture` | 0 or more | |     
| `media` | 0 or more | |     
| `scope` | 0 or more | |     
| `scroller` | 0 or more | |     
| `select` | 0 or more | |     
| `text` | 0 or more | |     
| `thumbnail`| 0 or more | |    
 
> The sequence of templates to instantiate and instructions to execute if the condition is true


###Setter Element

The `setter` element defines a property setter in the prototype of the constructor defined by a `class`, `behavior`, or `transition` element.   

####Tag  

| | | |
| --- | --- | --- |
| `setter` |

####Attributes  
| | | |
| --- | --- | --- |
| `id` | `identifier` | required |    
 
> The identifier of the property

<!--From CR re the following: Why "params" plural, vs. singular in its description?-->
 
| | | |
| --- | --- | --- |
| `params` | `identifier` | required |    
 
> The parameter of the setter

####CDATA  

JavaScript code, the body of the setter 

###Shade Element

The `shade` element configures an `effect` object to shade the image.   

####Tag  

| | | |
| --- | --- | --- |
| `shade` |
  
####Attributes  

| | | |
| --- | --- | --- |
| `opacity` | `number` |   
 
> The shade opacity, as a number between 0 and 1 (the default)
 
| | | |
| --- | --- | --- |
| `texture` | `reference` |    
 
> A reference to a `texture` object to use as a shade

###Skin Element

The `skin` element creates a `skin` object. A skin can either have `texture`, `x`, `y`, `width`, and `height` attributes and `margins`, `states`, `titles`, and `variants` elements or have a `color` attribute and a `borders` element.   

####Tag  

| | | |
| --- | --- | --- |
| `skin` |    

####Attributes

| | | |
| --- | --- | --- |
| `color` | ` string` | |    
 
> The skin's color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`transparent` by default)  
 
| | | |
| --- | --- | --- |
| `id` | `identifier` | required * |      
 
> The identifier of the `skin` object 

> &#42; A `skin` element nested in a template cannot have an `id` attribute.  
 
| | | |
| --- | --- | --- |
| `texture` | `reference` | |    
 
> A reference to a `texture` object  
 
| | | |
| --- | --- | --- |
| `x, y` | `number` | required * |     
| `width, height` | ` number ` | required * |    
 
> The bounds of the skin within its texture. 

> &#42; These attributes are required only if the skin has a `texture` attribute.

####Elements  

| | | |
| --- | --- | --- |
| `borders` | 0 or 1 | |     
| `margins` | 0 or 1 | |      
| `states` | 0 or 1 | |      
| `tiles` | 0 or 1 | |    
| `variants` | 0 or 1 | |  
 
> The skin's borders, margins, states, tiles, or variants

###Span Element

The `span` element creates a run inside a `block` element.    

####Tag  

| | | |
| --- | --- | --- |
| `span` |   

####Attributes 

| | | |
| --- | --- | --- |
| `string` | `expression` | |    
 
> The run's characters
 
| | | |
| --- | --- | --- |
| `style` | `reference` | |    
 
> The run's style, a reference to a `style` object. KinomaJS cascades the style of the `block` element.

###States Element

The `states` element sets the states offset of a `skin` object.    

####Tag  

| | | |
| --- | --- | --- |
| `states` |   

####Attributes

| | | |
| --- | --- | --- |
| `colors` | `string` (1 or more, separated by commas) |

<!--From CR re the following: Description of `colors` attribute is my guess; please verify or correct.-->
 
> The colors of the states, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`black` by default)
 
| | | |
| --- | --- | --- |
| `names` | `identifier` (1 or more, separated by commas) |
 
> The identifiers of the states
 
| | | |
| --- | --- | --- |
| `offset` | `number` | |    
 
> The vertical distance between states

###Style Element

The `style` element creates a `style` object.   

####Tag  

| | | |
| --- | --- | --- |
| `style` |   

####Attributes  

| | | |
| --- | --- | --- |
| `align` | `string` | |   
 
> The style's horizontal (`left`, `center`, `right`, or `justify`) and vertical (`top`, `middle`, or `bottom`) alignments, separated by a comma (`center` and `middle` by default) 
 
| | | |
| --- | --- | --- |
| `color` | `string` | |    

> The style's color, in [CSS3 color syntax](http://www.w3.org/TR/css3-color/#colorunits) (`black` by default)  
 
| | | |
| --- | --- | --- |
| `font` | `string` | |    

<!--From CR: Once the more detailed writeup of font syntax has been solidified in JavaScript Ref, I can copy it into the following--or should I instead refer to that document for more information?-->

> The style's font, as a string specifying *font-weight, font-size,* and/or *font-family* in [CSS font syntax](http://www.w3.org/TR/CSS21/fonts.html)  
 
| | | |
| --- | --- | --- |
| `id` | `identifier` | required * |    
 
> The identifier of the `style` object   
 
> &#42; A `style` element nested in a template cannot have an `id` attribute.   
 
| | | |
| --- | --- | --- |
| `indentation` | `number` | |    
 
> The style's indentation: the indentation of the first line of a block
 
| | | |
| --- | --- | --- |
| `leading` | `number` | |    
 
> The style's line height, or "leading": the distance between lines of a block. If 0 or unspecified, it is automatically calculated. Use a negative value to force a distance even if lines overlap.
 
| | | |
| --- | --- | --- |
| `lines` | `number` | |    

> The style's line count: the maximum number of lines in a block. If 0 or unspecified, all lines that fit in the container are included in it.

####Elements 

| | | |
| --- | --- | --- |
| `margins` | 0 or 1 | |    
 
> The style's margins
 
| | | |
| --- | --- | --- |
| `states` | 0 or 1 | |    
 
> The style's states

###Text Element

The `text` element defines or extends a template to instantiate a `text` object.   

####Tag  

| | | |
| --- | --- | --- |
| `text` | |     

####Attributes  

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)), plus the following. The `like` attribute, if any, must reference another `text` element.  
 
| | | |
| --- | --- | --- |
| `editable` | ` boolean` | |       
| `selectable` | ` boolean` | |      
 
> If `true`, the string is editable or selectable, respectively.   
 
| | | |
| --- | --- | --- |
| `string` | `expression` | |    
 
> The characters to render. When instantiating, KinomaJS evaluates the defined `string` attribute to set the corresponding property of the `text` object.

####Elements  

Same as for the `content` element (see [Elements](#content-element-elements) in the section [Content Element](#content-element)), plus: 
 
| | | |
| --- | --- | --- |
| `block` | 0 or more | |    
 
> The paragraphs to render. The `text` element cannot have both a `string` attribute and `block` elements.

###Texture Element

The `texture` element creates a `texture` object.   

####Attributes 
| | | |
| --- | --- | --- |
| `id` | `identifier` | required | 
   
> The identifier of the `texture` object
 
| | | |
| --- | --- | --- |
| `small` | `string` | |     
| `medium` | `string` | |    
| `large` | `string` | |    
 
> The relative path to the image asset

###Thumbnail Element

The `thumbnail` element defines or extends a template to instantiate a `thumbnail` object.   

####Tag  

| | | |
| --- | --- | --- |
| `thumbnail` | | 

####Attributes  

Same as for the `content` element (see [Attributes](#content-element-attributes) in the section [Content Element](#content-element)), plus the following. The `like` attribute, if any, must reference another `thumbnail` element.
 
| | | |
| --- | --- | --- |
| `aspect` | `string` | |    
 
> How the thumbnail is displayed
 
| | | |
| --- | --- | --- |
| `mime` | `expression` | |    
 
> The MIME type of the image
 
| | | |
| --- | --- | --- |
| `url` | `expression` | |    
 
> The URL of the image. Inside a program, the value of this attribute is merged with the URL of the application. Inside a module, the value of this attribute is merged with the URL of the module.

####Elements  

Same as for the `content` element (see [Elements](#content-element-elements) in the section [Content Element](#content-element)) 

###Tiles Element

The `tiles` element sets the tiles of a `skin` object.    

####Tag  

| | | |
| --- | --- | --- |
| `tiles` |   

####Attributes  

| | | |
| --- | --- | --- |
| `left` | `number` | |     
| `right` | `number` | |     
| `top` | `number` | |     
| `bottom` | `number` | |    
 
> The size of the left, right, top, and bottom tiles

###Transition Element

The `transition` element defines a `Transition` constructor and its prototype. Applications and shells use `Transition` constructors to create `transition` objects.    

####Tag  
| | | |
| --- | --- | --- |
| `transition` |   

####Attributes  
| | | |
| --- | --- | --- |
| `duration` | `number` | |      
 
> The duration of the `transition` object in milliseconds (250 by default)  
 
| | | |
| --- | --- | --- |
| `id` | `identifier` | required |    
 
> The identifier of this `Transition` constructor
 
| | | |
| --- | --- | --- |
| `like` | `reference` | |      
 
> A reference to a `Transition` constructor (`Transition` by default). The properties of the prototype of the referenced constructor are inherited by the prototype of the defined constructor.

####Elements  

| | | |
| --- | --- | --- |
| `field` | 0 or more | |     
| `getter` | 0 or more | |    
| `method` | 0 or more | |    
| `setter` | 0 or more | |   
 
> The properties of the prototype of this `Transition` constructor

###Variable Element

The `variable` element defines a variable in a program or a module.   

####Tag  

| | | |
| --- | --- | --- |
| `variable` |

####Attributes  

| | | |
| --- | --- | --- |
| `id` | `identifier` | required |    
 
> The identifier of the variable
 
| | | |
| --- | --- | --- |
| `value` | `expression` | |    
 
> The initial value of the variable

###Variants Element

The `variants` element sets the variants offset of a `skin` object.    

####Tag  

| | | |
| --- | --- | --- |
| `variants` |   

####Attributes  

| | | |
| --- | --- | --- |
| `names` | `identifier` (1 or more, separated by commas) |  
 
> The identifiers of the variants
 
| | | |
| --- | --- | --- |
| `offset` | `number` | |    
 
> The horizontal distance between variants

###Wrap Element

The `wrap` element inserts a `content` object inside a `block` element.    

####Tag  

| | | |
| --- | --- | --- |
| `wrap` |   

####Attributes  

| | | |
| --- | --- | --- |
| `alignment` | `string` | |     
 
> The alignment of the `content` object in the paragraph: `left` or `right` to float; `top`, `middle`, or `bottom` for inline 

####Elements  

| | | |
| --- | --- | --- |
| `canvas` | `1` | |     
| `column` | `1` | |     
| `container` | `1` | |     
| `content` | `1` | |     
| `label` | `1` | |     
| `layout` | `1` | |     
| `line` | `1` | |     
| `picture` | `1` | |     
| `media` | `1` | |     
| `scroller` | `1` | |     
| `text` | `1` | |     
| `thumbnail` | `1` | |    
 
> The template to instantiate to create the object to insert
