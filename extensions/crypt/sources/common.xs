<?xml version="1.0" encoding="UTF-8"?>
<!--
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
-->
<package>
	<patch prototype="Crypt">
		<object name="error" prototype="Error.prototype">
			<object name="subError" prototype="Crypt.error" pattern="subError"/>

			<!-- error code definitions for the Crypt.error object -->
			<number name="kCryptNoError"	value="0"/>
			<number name="kCryptMalformedInput"	value="1"/>
			<number name="kCryptInvalidAlgorithm"	value="2"/>
			<number name="kCryptUnsupportedAlgorithm"	value="3"/>
			<number name="kCryptRangeError"	value="4"/>	<!-- input parameter range error -->
			<number name="kCryptTypeError"	value="5"/>
			<number name="kCryptParameterError"	value="6"/>
			<number name="kCryptSystemError"	value="7"/>	<!-- an error raised from XS or Fsk core -->
			<number name="kCryptVerificationError"	value="8"/>
			<number name="kCryptKeyNotFound"	value="9"/>
			<number name="kCryptObjectNotFound"	value="10"/>
			<number name="kCryptMemoryFull"	value="11"/>
			<number name="kCryptNaNError"	value="12"/>
			<number name="kCryptUnimplemented"	value="13"/>
			<number name="kCryptNoRetrievalMethod"	value="14"/>
			<number name="kArithInsufficientTemporaryBuffer"	value="15"/>
			<number name="kArithDivideByZero"	value="16"/>
			<number name="kArithCalculationError"	value="17"/>
			<number name="kCryptUnknownError"	value="999"/>
		</object>
		<function name="Error" params="code, message, subError" prototype="Crypt.error">
			this.code = code;
			if (message)
				this.message = message;
			else
				this.message = "Crypt error";
			if (subError)
				this.subError = subError;
		</function>

		<function name="exit" params="code" c="xs_crypt_exit"/>
	</patch>
</package>