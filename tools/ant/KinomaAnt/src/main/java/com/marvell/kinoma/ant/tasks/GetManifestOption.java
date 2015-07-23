/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
package com.marvell.kinoma.ant.tasks;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.DocumentBuilder;

import org.w3c.dom.Document;

import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathFactory;

import java.io.File;


public class GetManifestOption extends Task {

	private String element;
	private String property;
	private boolean fail = false;

	public void setElement(String element) {
		this.element = element;
	}

	public void setProperty(String property) {
		this.property = property;
	}

	public void setFailonerror(boolean fail) {
		this.fail = fail;
	}


	public void execute() throws BuildException {
		String manifestFile = getProject().getProperty("fsk.manifest");
		String targetPlatform = getProject().getProperty("target.platform");
		if (manifestFile == null)
			throw new BuildException("fsk.manifest must be defined");
		DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();
		try {
			DocumentBuilder docBuilder = docBuilderFactory.newDocumentBuilder();
			Document doc = docBuilder.parse(new File(manifestFile));

			XPath xPath = XPathFactory.newInstance().newXPath();

			String value = (String)xPath.evaluate("/fsk/rootvm/options/" + element + "[@platform='" + targetPlatform + "']/@value", doc, XPathConstants.STRING);

			if (value != null &! value.isEmpty()) {
				getProject().setProperty(property, value);
			} else {
				if (this.fail == true) {
					throw new BuildException(element + " does not exists in the manifest for this platform");
				}
			}
		} catch (Exception e) {
			throw new BuildException(e.getMessage());
		}
	}

}
