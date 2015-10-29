<?xml version="1.0" encoding="UTF-8"?>
<!--
|     Copyright (C) 2010-2015 Marvell International Ltd.
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
<package script="true">
    <import href="kpr.xs" link="dynamic"/>

    <object name="Home">
        <!--
        @brief Manages collection of one or more homes.

        @discussion This class is responsible for managing a collection of homes.
        -->
        <object name="manager" c="KPR_Home_manager_destructor">
            <function name="get homes" c="KPR_Home_manager_get_homes" enum="false"/>
            <function name="get primaryHome" c="KPR_Home_manager_get_primaryHome" enum="false"/>
            <function name="get ready" c="KPR_Home_manager_is_ready" enum="false"/>

            <function name="get browser" c="KPR_Home_manager_get_browser" enum="false"/>

            <function name="addHome" params="name, complete" c="KPR_Home_manager_addHome"/>
            <function name="removeHome" params="home, complete" c="KPR_Home_manager_removeHome"/>
            <function name="setPrimaryHome" params="home, complete" c="KPR_Home_manager_setPrimaryHome"/>

            <function name="onHomeManagerReady" params="manager">
                application.delegate("onHomeManagerReady", manager);
            </function>

            <function name="onHomeManagerUpdate" params="manager">
                application.delegate("onHomeManagerUpdate", manager);
            </function>

            <function name="onHomeBrowserDidFindAccessory" params="browser, accessory">
                application.delegate("onHomeBrowserAddAccessory", browser, accessory);
            </function>

            <function name="onHomeBrowserDidLoseAccessory" params="browser, accessory">
                application.delegate("onHomeBrowserRemoveAccessory", browser, accessory);
            </function>

            <function name="onHomeDidUpdateName" params="home">
                application.delegate("onHomeDidUpdateName", home)
            </function>

            <function name="onHomeDidAddAccessory" params="home, accessory">
                application.delegate("onHomeDidAddAccessory", home, accessory)
            </function>

            <function name="onHomeDidRemoveAccessory" params="home, accessory">
                application.delegate("onHomeDidRemoveAccessory", home, accessory)
            </function>

            <function name="onHomeDidAddUser" params="home, user">
                application.delegate("onHomeDidAddUser", home, user)
            </function>

            <function name="onHomeDidRemoveUser" params="home, user">
                application.delegate("onHomeDidRemoveUser", home, user)
            </function>

            <function name="onHomeDidAddRoom" params="home, room">
                application.delegate("onHomeDidAddRoom", home, room)
            </function>

            <function name="onHomeDidRemoveRoom" params="home, room">
                application.delegate("onHomeDidRemoveRoom", home, room)
            </function>

            <function name="onHomeDidAddZone" params="home, zone">
                application.delegate("onHomeDidAddZone", home, zone)
            </function>

            <function name="onHomeDidRemoveZone" params="home, zone">
                application.delegate("onHomeDidRemoveZone", home, zone)
            </function>

            <function name="onHomeDidAddServiceGroup" params="home, serviceGroup">
                application.delegate("onHomeDidAddServiceGroup", home, serviceGroup)
            </function>

            <function name="onHomeDidRemoveServiceGroup" params="home, serviceGroup">
                application.delegate("onHomeDidRemoveServiceGroup", home, serviceGroup)
            </function>

            <function name="onHomeDidAddActionSet" params="home, actionSet">
                application.delegate("onHomeDidAddActionSet", home, actionSet)
            </function>

            <function name="onHomeDidRemoveActionSet" params="home, actionSet">
                application.delegate("onHomeDidRemoveActionSet", home, actionSet)
            </function>

            <function name="onHomeDidAddTrigger" params="home, trigger">
                application.delegate("onHomeDidAddTrigger", home, trigger)
            </function>

            <function name="onHomeDidRemoveTrigger" params="home, trigger">
                application.delegate("onHomeDidRemoveTrigger", home, trigger)
            </function>


            <function name="onHomeAccessoryDidUpdateName" params="accessory">
                application.delegate("onHomeAccessoryDidUpdateName", accessory);
            </function>

            <function name="onHomeAccessoryDidUpdateRoom" params="accessory, room">
                application.delegate("onHomeAccessoryDidUpdateRoom", accessory, room)
            </function>

            <function name="onHomeAccessoryDidUpdateServices" params="accessory">
                application.delegate("onHomeAccessoryDidUpdateServices", accessory);
            </function>

            <function name="onHomeAccessoryDidUpdateReachability" params="accessory">
                application.delegate("onHomeAccessoryDidUpdateReachability", accessory);
            </function>

            <function name="onHomeAccessoryDidUnblock" params="accessory">
                application.delegate("onHomeAccessoryDidUnblock", accessory)
            </function>

            <function name="onHomeAccessoryError" params="accessory, err">
                application.delegate("onHomeAccessoryError", accessory, err)
            </function>

            <function name="onHomeServiceDidUpdateName" params="service">
                application.delegate("onHomeServiceDidUpdateName", service);
            </function>

            <function name="onHomeServiceDidUpdateAssociatedServiceType" params="service">
                application.delegate("onHomeServiceDidUpdateAssociatedServiceType", service);
            </function>

            <function name="onHomeCharacteristicDidUpdateValue" params="characteristic">
                application.delegate("onHomeCharacteristicDidUpdateValue", characteristic);
            </function>

            <function name="onHomeRoomDidUpdateName" params="room">
                application.delegate("onHomeRoomDidUpdateName", room)
            </function>

            <function name="onHomeZoneDidUpdateName" params="zone">
                application.delegate("onHomeZoneDidUpdateName", zone)
            </function>

            <function name="onHomeZoneDidAddRoom" params="zone, room">
                application.delegate("onHomeZoneDidAddRoom", zone, room)
            </function>

            <function name="onHomeZoneDidRemoveRoom" params="zone, room">
                application.delegate("onHomeZoneDidRemoveRoom", zone, room)
            </function>

            <function name="onHomeServiceGroupDidUpdateName" params="serviceGroup">
                application.delegate("onHomeServiceGroupDidUpdateName", serviceGroup)
            </function>

            <function name="onHomeServiceGroupDidAddService" params="serviceGroup, service">
                application.delegate("onHomeServiceGroupDidAddService", serviceGroup, service)
            </function>

            <function name="onHomeServiceGroupDidRemoveService" params="serviceGroup, service">
                application.delegate("onHomeServiceGroupDidRemoveService", serviceGroup, service)
            </function>

            <function name="onHomeActionSetDidUpdateName" params="actionSet">
                application.delegate("onHomeActionSetDidUpdateName", actionSet)
            </function>

            <function name="onHomeActionSetDidUpdateActions" params="actionSet">
                application.delegate("onHomeActionSetDidUpdateActions", actionSet)
            </function>

            <function name="onHomeTriggerDidUpdateName" params="trigger">
                application.delegate("onHomeTriggerDidUpdateName", trigger)
            </function>

            <function name="onHomeTriggerDidUpdate" params="trigger">
                application.delegate("onHomeTriggerDidUpdate", trigger)
            </function>
        </object>


        <function name="Manager" params="behavior" prototype="Home.manager" c="KPR_Home_Manager"/>

        <!--
        @brief This class is used to discover new accessories in the home
        that have never been paired with and therefore not part of the home.
        -->
        <object name="browser" c="KPR_Home_browser_destructor">
            <function name="get accessories" c="KPR_Home_browser_get_accessories" enum="false"/>
            <function name="start" params="" c="KPR_Home_browser_start"/>
            <function name="stop" params="" c="KPR_Home_browser_stop"/>
        </object>

        <!--
        @brief Represents a home.

        @discussion This class represents a home and is the entry point to communicate and
        configure different accessories in the home. This is also used to manage
        all the rooms, zones, service groups, users, triggers, and action sets in
        the home.
        -->
        <object name="home" c="KPR_Home_home_destructor">
            <!--

            home
            -->
            <function name="get name" c="KPR_Home_home_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_home_updateName"/>
            <function name="get primary" c="KPR_Home_home_is_primary" enum="false"/>

            <function name="get accessories" c="KPR_Home_home_get_accessories" enum="false"/>
            <function name="addAccessory" params="accessory, complete" c="KPR_Home_home_addAccessory"/>
            <function name="removeAccessory" params="accessory, complete" c="KPR_Home_home_removeAccessory"/>
            <function name="assignAccessory" params="accessory, room, complete" c="KPR_Home_home_assignAccessory"/>
            <function name="queryServices" params="type" c="KPR_Home_home_queryServices"/>
            <function name="unblockAccessory" params="accessory, complete" c="KPR_Home_home_unblockAccessory"/>

            <function name="get users" c="KPR_Home_home_get_users" enum="false"/>
            <function name="addUser" params="complete" c="KPR_Home_home_addUser"/>
            <function name="removeUser" params="user, complete" c="KPR_Home_home_removeUser"/>

            <function name="get rooms" c="KPR_Home_home_get_rooms" enum="false"/>
            <function name="addRoom" params="name, complete" c="KPR_Home_home_addRoom"/>
            <function name="removeRoom" params="room, complete" c="KPR_Home_home_removeRoom"/>

            <function name="get zones" c="KPR_Home_home_get_zones" enum="false"/>
            <function name="addZone" params="name, complete" c="KPR_Home_home_addZone"/>
            <function name="removeZone" params="zone, complete" c="KPR_Home_home_removeZone"/>

            <function name="get serviceGroups" c="KPR_Home_home_get_serviceGroups" enum="false"/>
            <function name="addServiceGroup" params="name, complete" c="KPR_Home_home_addServiceGroup"/>
            <function name="removeServiceGroup" params="serviceGroups, complete" c="KPR_Home_home_removeServiceGroup"/>

            <function name="get actionSets" c="KPR_Home_home_get_actionSets" enum="false"/>
            <function name="addActionSet" params="name, complete" c="KPR_Home_home_addActionSet"/>
            <function name="removeActionSet" params="actionSet, complete" c="KPR_Home_home_removeActionSet"/>
            <function name="executeActionSet" params="actionSet, complete" c="KPR_Home_home_executeActionSet"/>

            <function name="get triggers" c="KPR_Home_home_get_triggers" enum="false"/>
            <function name="addTimerTrigger" params="fireDate, recurrence, complete" c="KPR_Home_home_addTimerTrigger"/>
            <function name="removeTrigger" params="trigger, complete" c="KPR_Home_home_removeTrigger"/>
        </object>

        <!--
        @abstract Represent an accessory in the home.

        @discussion This class represents an accessory in the home. There is a one to
        one relationship between a physical accessory and an object of this
        class. An accessory is composed of one or more services.
        -->
        <object name="accessory" c="KPR_Home_accessory_destructor">

            <function name="get name" c="KPR_Home_accessory_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_accessory_updateName"/>
            <function name="get identifier" c="KPR_Home_accessory_get_identifier" enum="false"/>
            <function name="get reachable" c="KPR_Home_accessory_is_reachable" enum="false"/>
            <function name="get standalone" c="KPR_Home_accessory_is_standalone" enum="false"/>
            <function name="get bridged" c="KPR_Home_accessory_is_bridged" enum="false"/>
            <function name="get bridge" c="KPR_Home_accessory_is_bridge" enum="false"/>
            <function name="get bridgedAccessoryIdentifiers" c="KPR_Home_accessory_get_bridgedAccessoryIdentifiers" enum="false"/>

            <function name="get room" c="KPR_Home_accessory_get_room" enum="false"/>

            <function name="get services" c="KPR_Home_accessory_get_services" enum="false"/>

            <function name="get blocked" c="KPR_Home_accessory_is_blocked" enum="false"/>

            <function name="identify" params="complete" c="KPR_Home_accessory_identify"/>
        </object>

        <!--
        @brief Represents a service provided by an accessory.

        @discussion This class represents a service provided by an accessory in the home.
        A service is composed of one or more characteristics that can be
        modified.
        -->
        <object name="service" c="KPR_Home_service_destructor">
            <function name="get accessory" c="KPR_Home_service_get_accessory" enum="false"/>
            <function name="get type" c="KPR_Home_service_get_type" enum="false"/>
            <function name="get name" c="KPR_Home_service_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_service_updateName"/>
            <function name="get associatedType" c="KPR_Home_service_get_associatedType" enum="false"/>
            <function name="updateAssociatedType" params="type, complete" c="KPR_Home_service_updateAssociatedType"/>
            <function name="get characteristics" c="KPR_Home_service_get_characteristics" enum="false"/>
        </object>

        <object name="characteristic" c="KPR_Home_characteristic_destructor">
            <function name="get hash" enum="false"/>
            <function name="get service" c="KPR_Home_characteristic_get_service" enum="false"/>
            <function name="get type" c="KPR_Home_characteristic_get_type" enum="false"/>
            <function name="get properties" c="KPR_Home_characteristic_get_properties" enum="false"/>
            <function name="get metadata" c="KPR_Home_characteristic_get_metadata" enum="false"/>

            <function name="get readable" c="KPR_Home_characteristic_is_readable" enum="false"/>
            <function name="get writable" c="KPR_Home_characteristic_is_writable" enum="false"/>
            <function name="get supportsEventNotification" c="KPR_Home_characteristic_is_supportsEventNotification" enum="false"/>

            <function name="get value" c="KPR_Home_characteristic_get_value" enum="false"/>
            <function name="writeValue" params="value, complete" c="KPR_Home_characteristic_writeValue"/>
            <function name="readValue" params="complete" c="KPR_Home_characteristic_readValue"/>

            <function name="get notificationEnabled" c="KPR_Home_characteristic_is_notificationEnabled" enum="false"/>
            <function name="enableNotification" params="complete" c="KPR_Home_characteristic_enableNotification"/>
            <function name="disableNotification" params="complete" c="KPR_Home_characteristic_disableNotification"/>
        </object>

        <!--
        @brief This class defines the metadata for a characteristic. Metadata provides
        further information about a characteristic’s value, which can be used
        for presentation purposes.
        -->
        <object name="characteristicMetadata" c="KPR_Home_characteristicMetadata_destructor">
            <function name="get minimumValue" c="KPR_Home_characteristicMetadata_get_minimumValue" enum="false"/>
            <function name="get maximumValue" c="KPR_Home_characteristicMetadata_get_maximumValue" enum="false"/>
            <function name="get stepValue" c="KPR_Home_characteristicMetadata_get_stepValue" enum="false"/>
            <function name="get maxLength" c="KPR_Home_characteristicMetadata_get_maxLength" enum="false"/>
            <function name="get format" c="KPR_Home_characteristicMetadata_get_format" enum="false"/>
            <function name="get units" c="KPR_Home_characteristicMetadata_get_units" enum="false"/>
            <function name="get manufacturerDescription" c="KPR_Home_characteristicMetadata_get_manufacturerDescription" enum="false"/>
        </object>

        <!-- @brief This class describes a room in the home. -->
        <object name="room" c="KPR_Home_room_destructor">
            <function name="get name" c="KPR_Home_room_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_room_updateName"/>

            <function name="get accessories" c="KPR_Home_room_get_accessories" enum="false"/>
        </object>

        <!--
        @brief Used to describe a collection of Room objects

        @discussion This class is used to group a collection of rooms.
        This allows for association of a set of rooms into a group.
        Eg. "Living Room" and "Kitchen" rooms can be grouped together
        in the "Downstairs" zone.
        -->
        <object name="zone" c="KPR_Home_zone_destructor">
            <function name="get name" c="KPR_Home_zone_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_zone_updateName"/>

            <function name="get rooms" c="KPR_Home_zone_get_rooms" enum="false"/>
            <function name="addRoom" params="room, complete" c="KPR_Home_zone_addRoom"/>
            <function name="removeRoom" params="room, complete" c="KPR_Home_zone_removeRoom"/>
        </object>

        <!--
        @abstract Used to describe a collection of Service objects

        @discussion This class is used to group a collection of Service objects.
        This allows for association of a set of accessory services into a group.
        Eg. A collection of lights can be grouped as the "Desk Lamps" service group.
        -->
        <object name="serviceGroup" c="KPR_Home_serviceGroup_destructor">
            <function name="get name" c="KPR_Home_serviceGroup_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_serviceGroup_updateName"/>

            <function name="get services" c="KPR_Home_serviceGroup_get_services" enum="false"/>
            <function name="addService" params="service, complete" c="KPR_Home_serviceGroup_addService"/>
            <function name="removeService" params="service, complete" c="KPR_Home_serviceGroup_removeService"/>
        </object>

        <!--
        @brief This class represents a collection of action objects that can be executed.
        The order of execution of these actions is undefined.
        -->
        <object name="actionSet" c="KPR_Home_actionSet_destructor">
            <function name="get name" c="KPR_Home_actionSet_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_actionSet_updateName"/>

            <function name="get actions" c="KPR_Home_actionSet_get_actions" enum="false"/>
            <function name="addCharacteristicWriteAction" params="characteristic, value, complete" c="KPR_Home_actionSet_addCharacteristicWriteAction"/>
            <function name="removeAction" params="action, complete" c="KPR_Home_actionSet_removeAction"/>

            <function name="get executing" c="KPR_Home_actionSet_is_executing" enum="false"/>
        </object>

        <!--
        @brief This class is used to represent a generic action.

        CharacteristicWriteAction
        @brief This class is used to represent an entry in an action set that writes a specific
        value to a characteristic.
        -->
        <object name="characteristicWriteAction" c="KPR_Home_characteristicWriteAction_destructor">
            <function name="get characteristic" c="KPR_Home_characteristicWriteAction_get_characteristic" enum="false"/>
            <function name="get targetValue" c="KPR_Home_characteristicWriteAction_get_targetValue" enum="false"/>
            <function name="updateTargetValue" params="value, complete" c="KPR_Home_characteristicWriteAction_updateTargetValue"/>
        </object>

        <!--
        @brief Represents a trigger event.

        @discussion This class describes a trigger which is an event that can
        be used to execute one or more action sets when the event fires.
        -->
        <object name="timerTrigger" c="KPR_Home_timerTrigger_destructor">
            <function name="get name" c="KPR_Home_timerTrigger_get_name" enum="false"/>
            <function name="updateName" params="name, complete" c="KPR_Home_timerTrigger_updateName"/>

            <function name="get enabled" c="KPR_Home_timerTrigger_is_enabled" enum="false"/>
            <function name="enable" params="complete" c="KPR_Home_timerTrigger_enable"/>
            <function name="disable" params="complete" c="KPR_Home_timerTrigger_disable"/>

            <function name="get actionSets" c="KPR_Home_timerTrigger_get_actionSets" enum="false"/>
            <function name="addActionSet" params="actionSet, complete" c="KPR_Home_timerTrigger_addActionSet"/>
            <function name="removeActionSet" params="actionSet, complete" c="KPR_Home_timerTrigger_removeActionSet"/>

            <function name="get fireDate" c="KPR_Home_timerTrigger_get_fireDate" enum="false"/>
            <function name="get recurrence" c="KPR_Home_timerTrigger_get_recurrence" enum="false"/>
            <function name="get lastFireDate" c="KPR_Home_timerTrigger_get_lastFireDate" enum="false"/>

            <function name="updateFireDate" params="fireDate, completion" c="KPR_Home_timerTrigger_updateFireDate"/>
            <function name="updateRecurrence" params="recurrence, completion" c="KPR_Home_timerTrigger_updateRecurrence"/>
        </object>

        <!--
        @brief An HMUser object represents a person in the home who may have access to control
        accessories and services in the home.
        -->
        <object name="user" c="KPR_Home_user_destructor">
            <function name="get name" c="KPR_Home_user_get_name" enum="false"/>
        </object>
    </object>
</package>

