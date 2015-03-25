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
package com.kinoma.kinomaplay;

import android.content.Context;
import android.net.nsd.NsdManager;
import android.net.nsd.NsdManager.DiscoveryListener;
import android.net.nsd.NsdManager.RegistrationListener;
import android.net.nsd.NsdManager.ResolveListener;
import android.net.nsd.NsdServiceInfo;
import android.util.Log;

import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.net.InetAddress;

public class KprZeroconfAndroid
{
	private static final String TAG = "KprZeroconfJava";
	private static NsdManager sNsdManager = null;

	static HashMap<String, KprZeroconfAdvertisement> advertisements = new HashMap<String, KprZeroconfAdvertisement>();

	static class KprZeroconfAdvertisement implements RegistrationListener
	{
		private String serviceType;

		public KprZeroconfAdvertisement(Context context, String serviceType, String serviceName, int port) {
			NsdServiceInfo serviceInfo;
			this.serviceType = serviceType;
	        serviceInfo = new NsdServiceInfo();
	        serviceInfo.setServiceName(serviceName);
	        serviceInfo.setServiceType(serviceType);
	        serviceInfo.setPort(port);
			Log.i(TAG, "new KprZeroconfAdvertisement - " + serviceInfo);
			sNsdManager.registerService(serviceInfo, NsdManager.PROTOCOL_DNS_SD, this);
		}
		public void tearDown() {
			Log.i(TAG, "tearDown Advertisement - " + this.serviceType);
			sNsdManager.unregisterService(this);
			this.serviceType = null;
		}

		// RegistrationListener
		@Override
		public void onRegistrationFailed(NsdServiceInfo serviceInfo, int errorCode)
		{
			Log.i(TAG, "onRegistrationFailed - " + serviceInfo + " -> " + errorCode);
		}

		public void onServiceRegistered (NsdServiceInfo serviceInfo)
		{
			Log.i(TAG, "onServiceRegistered - " + serviceInfo);
			KprZeroconfAndroid.serviceRegistered(this.serviceType, serviceInfo.getServiceName(), serviceInfo.getPort());
		}

		public void onServiceUnregistered (NsdServiceInfo serviceInfo)
		{
			Log.i(TAG, "onServiceUnregistered - " + serviceInfo);
		}

		public void onUnregistrationFailed(NsdServiceInfo serviceInfo, int errorCode)
		{
			Log.i(TAG, "onUnregistrationFailed - " + serviceInfo + " -> " + errorCode);
		}

	}

	static HashMap<String, KprZeroconfBrowser> browsers = new HashMap<String, KprZeroconfBrowser>();
	static KprZeroconfResolver resolver = new KprZeroconfResolver();

	static class KprZeroconfPendingService implements ResolveListener
	{
		private NsdServiceInfo service;
		private KprZeroconfBrowser browser;
		public KprZeroconfPendingService(NsdServiceInfo service, KprZeroconfBrowser browser) {
			this.service = service;
			this.browser = browser;
		}

		public Boolean compare(NsdServiceInfo service, KprZeroconfBrowser browser) {
			return (this.browser == browser) && (this.service.getServiceName().equals(service.getServiceName()));
		}

		// ResolveListener
		@Override
		public void onServiceResolved(NsdServiceInfo service)
		{
			String ip = service.getHost().getHostAddress();
			String name = service.getServiceName().replace("\\032", " ");
			Log.i(TAG, "onServiceResolved - " + this.browser.serviceType + " " + name + " " + ip + ":" + service.getPort());
			KprZeroconfAndroid.serviceUp(this.browser.serviceType, name, service.getHost().getHostName(), ip, service.getPort());
			resolver.resolved(this);
		}

		@Override
		public void onResolveFailed(NsdServiceInfo service, int errorCode)
		{
			Log.i(TAG, "onResolveFailed - " + service + " -> " + errorCode);
			resolver.resolved(this);
		}
	}

	static class KprZeroconfResolver
	{
		private List<KprZeroconfPendingService> pendingList = new ArrayList<KprZeroconfPendingService>();
		private KprZeroconfPendingService resolving;

		public void addService(NsdServiceInfo service, KprZeroconfBrowser browser) {
			Boolean found = (resolving != null) && resolving.compare(service, browser);
			if (!found) {
				for (int i = pendingList.size() - 1; (i >= 0) && !found; i--) {
					if (this.pendingList.get(i).compare(service, browser))
						found = true;
				}
			}
			Log.i(TAG, "addService - " + service + " found = " + found);
			if (!found) {
				KprZeroconfPendingService pending = new KprZeroconfPendingService(service, browser);
				this.pendingList.add(pending);
				this.resolve();
			}
		}
	
		public void removeService(NsdServiceInfo service, KprZeroconfBrowser browser) {
			Log.i(TAG, "removeService - " + service + " " + browser);
			Boolean found = (resolving != null) && resolving.compare(service, browser);
			if (!found) {
				for (int i = pendingList.size() - 1; (i >= 0) && !found; i--) {
					Log.i(TAG, i + " compare - " + this.pendingList.get(i).service);
					if (this.pendingList.get(i).compare(service, browser)) {
						found = true;
						Log.i(TAG, " -> removeService - " + service + " i = " + i);
						this.pendingList.remove(i);
					}
				}
			}
		}
	
		public void resolve() {
			if (sNsdManager == null) {
				this.resolving = null;
				this.pendingList.clear();
			}
			else if (this.resolving == null) {
				if (!this.pendingList.isEmpty()) {
					KprZeroconfPendingService pending = this.pendingList.get(0);
					this.pendingList.remove(0);
					this.resolving = pending;
					Log.i(TAG, "START RESOLVING - " + pending);
					sNsdManager.resolveService(pending.service, pending);
				}
			}
		}

		public void resolved(KprZeroconfPendingService pending) {
			this.resolving = null;
			this.resolve();
		}
	}

	static class KprZeroconfBrowser implements DiscoveryListener
	{
		private String serviceType;

		public KprZeroconfBrowser(Context context, String serviceType) {
			Log.i(TAG, "new KprZeroconfBrowser - " + serviceType);
			this.serviceType = serviceType;
			sNsdManager.discoverServices(serviceType, NsdManager.PROTOCOL_DNS_SD, this);
		}

		public void tearDown() {
			Log.i(TAG, "tearDown Browser - " + this.serviceType);
			sNsdManager.stopServiceDiscovery(this);
			this.serviceType = null;
		}

		// DiscoveryListener
		@Override
		public void onDiscoveryStarted(String serviceType)
		{
			Log.i(TAG, "onDiscoveryStarted - " + serviceType);
		}

		@Override
		public void onStartDiscoveryFailed(String serviceType, int errorCode)
		{
			Log.i(TAG, "onStartDiscoveryFailed - " + serviceType + ", " + errorCode);
		}

		@Override
		public void onDiscoveryStopped(String serviceType)
		{
			Log.i(TAG, "onDiscoveryStopped - " + serviceType);
		}

		@Override
		public void onStopDiscoveryFailed(String serviceType, int errorCode)
		{
			Log.i(TAG, "onStartDiscoveryFailed - " + serviceType + ", " + errorCode);
		}

		@Override
		public void onServiceFound(NsdServiceInfo service)
		{
			if (this.serviceType != null) {
				Log.i(TAG, "onServiceFound - " + this.serviceType + " " + service.getServiceName());
				String name = service.getServiceName().replace("\\032", " ");
				// KprZeroconfAndroid.serviceUp(this.serviceType, name, "unknown", "0.0.0.0", service.getPort());
				resolver.addService(service, this);
			}
		}

		@Override
		public void onServiceLost(NsdServiceInfo service)
		{
			if (this.serviceType != null) {
				Log.i(TAG, "onServiceLost - " + service.getServiceName());
			//	resolver.removeService(service, this);
				KprZeroconfAndroid.serviceDown(this.serviceType, service.getServiceName());
			}
		}			
	}

	static void startNSD(Context context) {
		Log.i(TAG, "startNSD");
		sNsdManager = (NsdManager)context.getSystemService(Context.NSD_SERVICE);
	}
	
	static void stopNSD(Context context) {
		Log.i(TAG, "stopNSD");
		sNsdManager = null;
	}

	// advertisement

	public static native void serviceRegistered(String type, String name, int port);

	public static void addAdvertisement(Context context, String serviceType, String serviceName, int port) {
		String tag = serviceType + "#" + port;
		if ((sNsdManager == null) && (advertisements.size() == 0)) {
			startNSD(context);
		}
		if (!advertisements.containsKey(tag)) {
			Log.i(TAG, "addAdvertisement - " + tag);
			KprZeroconfAdvertisement advertisement = new KprZeroconfAdvertisement(context, serviceType, serviceName, port);
			advertisements.put(tag, advertisement);
		}
	}

	public static void removeAdvertisement(Context context, String serviceType, String serviceName, int port) {
		String tag = serviceType + "#" + port;
		if (advertisements.containsKey(tag)) {
			Log.i(TAG, "removeAdvertisement - " + tag);
			advertisements.get(tag).tearDown();
			advertisements.remove(tag);
		}
		if ((browsers.size() == 0) && (advertisements.size() == 0)) {
			stopNSD(context);
		}
	}

	// browser

	public static native void serviceUp(String type, String name, String hostname, String ip, int port);
	public static native void serviceDown(String type, String name);

	public static void addService(Context context, String serviceType) {
		if ((sNsdManager == null) && (browsers.size() == 0)) {
			startNSD(context);
		}
		if (!browsers.containsKey(serviceType)) {
			Log.i(TAG, "addService - " + serviceType);
			KprZeroconfBrowser browser = new KprZeroconfBrowser(context, serviceType);
			browsers.put(serviceType, browser);
		}
	}
	public static void removeService(Context context, String serviceType) {
		if (browsers.containsKey(serviceType)) {
			Log.i(TAG, "removeService - " + serviceType);
			browsers.get(serviceType).tearDown();
			browsers.remove(serviceType);
		}
		if ((browsers.size() == 0) && (advertisements.size() == 0)) {
			stopNSD(context);
		}
	}
};
