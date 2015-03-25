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

import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.EditText;
import android.widget.TextView;

class FskEditText extends EditText {
	InputConnection teIC = null;

    public FskEditText(Context context, AttributeSet attrs) {
        super(context, attrs);

		// TODO Auto-generated constructor stub
	}

	//public boolean onCheckIsTextEditor() { return true; }
	
	public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
//      	android:imeOptions="actionDone|flagNoExtractUi|flagNoAccessoryAction|flagNoEnterAction"
       	outAttrs.imeOptions |= EditorInfo.IME_ACTION_DONE;
		outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_EXTRACT_UI;
		outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_ACCESSORY_ACTION;
		outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_ENTER_ACTION;
		outAttrs.inputType = EditorInfo.TYPE_TEXT_FLAG_AUTO_CORRECT;

		teIC = new BaseInputConnection(this, false);
		return teIC;
	} 
	
}
