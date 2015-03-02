/*
     Copyright (C) 2010-2015 Marvell International Ltd.
     Copyright (C) 2002-2010 Kinoma, Inc.

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
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


public class GetManifestVar extends Task {

	private String var;
	private String name;
	private boolean fail = false;

	public void setVariable(String var) {
		this.var = var;
	}

	public void setName(String name) {
		this.name = name;
	}

	public void setFailonerror(boolean fail) {
		this.fail = fail;
	}


	public void execute() throws BuildException {
		String manifestFile = getProject().getProperty("fsk.manifest");
		if (manifestFile == null)
			throw new BuildException("fsk.manifest must be defined");
		DocumentBuilderFactory docBuilderFactory = DocumentBuilderFactory.newInstance();
		try {
			DocumentBuilder docBuilder = docBuilderFactory.newDocumentBuilder();
			Document doc = docBuilder.parse(new File(manifestFile));

			XPath xPath = XPathFactory.newInstance().newXPath();

			String value = (String)xPath.evaluate("/fsk/rootvm/environment/variable[@name='" + var + "']/@value", doc, XPathConstants.STRING);

			if (value != null &! value.isEmpty()) {
				getProject().setProperty(name, value);
			} else {
				if (this.fail == true) {
					throw new BuildException(var + " does not exists in the manifest");
				}
			}
		} catch (Exception e) {
			throw new BuildException(e.getMessage());
		}
	}

}
