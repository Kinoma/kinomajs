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
package com.kinoma.kinomaplay;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

public class FskProperties 
{
	static private Properties		mRuntimeProperties;
	
	static private FskProperties	sInstance;
	
	private FskProperties() {
		mRuntimeProperties = new Properties();
	}
	
	private Properties getProperties() {
		return mRuntimeProperties;
	}
	
	private String getProperty( String name ) {
		return getInstance().getProperties().getProperty( name );
	}
	
	private String getProperty( String name, String defaultValue ) {
		return getInstance().getProperties().getProperty( name, defaultValue );
	}
	
	private boolean getProperty( String name, boolean defaultValue ) 
	{
		if( getInstance().getProperties().containsKey( name ) )
			return Boolean.parseBoolean( getInstance().getProperties().getProperty( name ) );
		else
			return defaultValue;
	}
	
	private int getProperty( String name, int defaultValue ) 
	{
		if( getInstance().getProperties().containsKey( name ) )
			return Integer.parseInt( getInstance().getProperties().getProperty( name ) );
		else
			return defaultValue;
	}
	
	static public FskProperties getInstance()
	{
		if( sInstance == null )
			sInstance = new FskProperties();

		return sInstance;
	}
	
	static public void load( InputStream is ) throws IOException {
		getInstance().getProperties().load( is );
	}

	static public boolean isOpenGL() {
		return getInstance().getProperty( "use.openGL", false );
	}
}
