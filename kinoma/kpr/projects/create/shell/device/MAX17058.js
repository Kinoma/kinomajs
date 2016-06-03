//@module
/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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

exports.pins = {
    microUSBCharger: {type: "Digital", direction: "input", pin: 1027}
}

exports.configure = function() {
    try {
        this.chargingBit0 = PINS.create({type: "Digital", pin: 1000, direction: "input"});
        this.chargingBit1 = PINS.create({type: "Digital", pin: 1001, direction: "input"});
        this.chargingBit0.init();
        this.chargingBit1.init();
        this.hasChargingBits = true;
    }
    catch(e) {
        trace("EXCEPTION with charging bits\n");
    	this.hasChargingBits = false;
    }

    this.microUSBCharger.init();

    // note: init fails if the battery is not charged or not present
    this.gasGauge = undefined;
}

exports.close = function() {
    this.gasGauge.close();
}

exports.read = function() {
    var result = {
        when: Date.now(),
        vcell: undefined,
        soc: undefined,
        hasChargingBits: this.hasChargingBits,
        chargeState: this.hasChargingBits ? ((this.chargingBit0.read() << 1) | this.chargingBit1.read()) : 0,
        microUSBCharger: !this.microUSBCharger.read()
    }

    if (!this.gasGauge)
        this.gasGauge = gasGauge();
    if (this.gasGauge) {
        try {
            result.vcell = swapWord(this.gasGauge.readWordDataSMB(0x02));
            result.soc = swapWord(this.gasGauge.readWordDataSMB(0x04));
            result.batteryLevel = (result.soc / 512) / 100;      // ini.bits is 19, so divide by 512 - percent from 0 to 1
        }
        catch (e) {
            trace("failed to read gas gauge, closing.\n");
            this.gasGauge.close();
            this.gasGauge = null;
        }
    }

    return result;
}

exports.chargerDetect = function() {
    return !this.microUSBCharger.read();
}

exports.chargerStatus = function() {
    return this.hasChargingBits ? ((this.chargingBit0.read() << 1) | this.chargingBit1.read()) : 0
}

/*
    For Maxim 17058
*/
function gasGauge() {
    var id, gasGauge;

    /*
        .ini data for K4
    */
    var ini = {
        EmptyAdjustment: 0,
        FullAdjustment: 100,
        RCOMP0: 100,
        TempCoUp: -1,
        TempCoDown: -1.775,
        OCVTest: 55696,
        SOCCheckA: 227,
        SOCCheckB: 229,
        bits: 19,
        data: [ 0xA3, 0xD0, 0xAD, 0x20, 0xB3, 0x50, 0xB9, 0x30, 0xBB, 0x10, 0xBB, 0xC0, 0xBC, 0x30, 0xBC, 0xA0,
                0xBD, 0x40, 0xC1, 0x00, 0xC2, 0xC0, 0xC4, 0xF0, 0xC6, 0x40, 0xC8, 0xE0, 0xCB, 0x30, 0xCF, 0x90,
                0x04, 0x20, 0x0F, 0x40, 0x0A, 0x20, 0x1E, 0x00, 0x45, 0xE0, 0x7B, 0xC0, 0x69, 0xE0, 0x49, 0xE0,
                0x0A, 0xC0, 0x30, 0x00, 0x1A, 0x20, 0x15, 0x40, 0x11, 0x00, 0x14, 0x00, 0x0C, 0x00, 0x0C, 0x00 ]
    };

    try {
        gasGauge = PINS.create({type: "I2C", address: 0x36, bus: 0});
        gasGauge.init();

        id = swapWord(gasGauge.readWordDataSMB(0x08));
    }
    catch (e) {
        trace("Exception instantating gas gauge\n");
    }

    if ((0x0011 != id) && (0x0012 != id)) {
        trace("Unrecognized gasGauge version " + id + "\n");
        return;
    }
    
    do {
        // Step 1: unlock model access
        ktrace("unlock model access \n");
        gasGauge.writeWordDataSMB(0x3E, swapWord(0x4A57));

        // Step 2: read OCV
        ktrace("read OCV \n");
        var original_OCV = swapWord(gasGauge.readWordDataSMB(0x0E));

        // Step 2.5: verify OCV
        ktrace("verify original_OCV " + original_OCV.toString(16) + "\n");
        if (0xffff != original_OCV)
            break;

        trace("Bad original_OCV after unlock. Retry. \n");
    } while (true);

    // Step 2.5.1: Send POR signal (17058/59 only)
    do {
        ktrace("Send POR signal \n");
        gasGauge.writeWordDataSMB(0x3E, swapWord(0x5400));
        gasGauge.writeWordDataSMB(0x3E, swapWord(0x4A57));
        var unlock_test_OCV_2 = swapWord(gasGauge.readWordDataSMB(0x0E));
        ktrace("Verify unlock_test_OCV_2 " + unlock_test_OCV_2.toString(16) + "\n");
    } while( 0xffff == unlock_test_OCV_2);

    var configuring;
    do {
        // Step 3: write OCV
        ktrace("write OCVTest " + ini.OCVTest.toString(16) +"\n");
        gasGauge.writeWordDataSMB(0x0E, swapWord(ini.OCVTest));

        // Step 4: Write RCOMP to max value (N/A on 17058)

        // Step 5: Write the model
        ktrace("Write the model \n");
        for (var i = 0; i < 4; i++)
            gasGauge.writeBlockDataSMB(0x40 + (i * 16), ini.data.slice(i * 16, (i + 1) * 16));

        // Step 6: Delay (N/A on 17058)

        // Step 7: Write OCV from INI
        ktrace("write OCVTest " + ini.OCVTest.toString(16) +"\n");
        gasGauge.writeWordDataSMB(0x0E, swapWord(ini.OCVTest));

        // Step 7.1: Disable Hibernate (N/A on 17058)
        
        // Step 7.2: Lock Model Access
        ktrace("Lock Model Access\n");
        gasGauge.writeWordDataSMB(0x3E, 0);
        
        // Step 8: delay between 150 and 600ms
        ktrace("delay\n");
        sensorUtils.mdelay(150);
        
        // Step 9: read SOC registers to check for expected result
        ktrace("read SOC registers\n");
        var soc = swapWord(gasGauge.readWordDataSMB(0x04));
        var SOC1 = (soc >> 8) & 0x0ff, SOC2 = soc & 0x0ff;
        ktrace("SOC1 " + SOC1 + ", SOC2 " + SOC2 + "\n");
        if ((SOC1 >= ini.SOCCheckA) && (SOC1 <= ini.SOCCheckB)) {
            ktrace("SOCCHECK OK\n");
            configuring = false;
        }
        else {
            /// go back to step 3 to try again.
            trace("SOCCHECK FAILED\n");
            configuring = true;
        }
    } while (configuring);

    // Step 9.1: clean-up model load
    ktrace("clean-up model load - unlock model again\n");
    gasGauge.writeWordDataSMB(0x3E, swapWord(0x4A57));

    // Step 10: restore CONFIG and OCV
    ktrace("restore CONFIG and OCV\n");
    gasGauge.writeWordDataSMB(0x0C, swapWord((ini.RCOMP0 << 8) | 0x1F));
    gasGauge.writeWordDataSMB(0x0E, swapWord(original_OCV));
    
    // Step 11: lock model access
    ktrace("Lock Model Access again\n");
    gasGauge.writeWordDataSMB(0x3E, 0);
    sensorUtils.mdelay(150);

    return gasGauge;
}

function swapWord(word) {
    return ((word >> 8) & 0xff) | ((word & 0xff) << 8)
}

function ktrace(message)
{
}

/*

    States:
    
        - no battery detected. or no charge. (power connected)
        - fully charged (power connected)
        - charging (power connected)
        - discharging (no power connected)

*/


/*
    GPIO 0 -> STAT1 on AAT3620
    GPIO 1 -> STAT2 on AAT3620
    (STAT 1 is blinking)
    charge state:
        00 = precharge (battery is too depleted so charging at 10% current to avoid overheating) (never seen!)
        01 = charging
        10 = charging complete (this happens!)
        11 = not charging
*/
