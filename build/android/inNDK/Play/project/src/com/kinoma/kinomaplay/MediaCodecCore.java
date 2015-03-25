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

/*import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;*/
import java.nio.ByteBuffer;

import android.util.Log;
import android.annotation.SuppressLint;
import android.media.MediaCodec;
import android.media.MediaCodecList;
import android.media.MediaCodecInfo; 

@SuppressLint("SdCardPath")
public class MediaCodecCore {

	private static final String ACTIVITY_TAG = "MediaCodecCoreLog";

    public int codec_status = 0;
	public int crop_lt      = 0;
	private byte[] outData;
	private int outSizeset  = 0;
	/* glabal variables */
	private ByteBuffer[] inputBuffers = null;
	private ByteBuffer[] outputBuffers = null;
	private MediaCodec.BufferInfo bufferInfo = null;
    private int g_outputBufferIndex;
    private MediaCodec mCodec;

	public int CodecFormatAvailable(String CodecType, int src_format) {
		Log.d(MediaCodecCore.ACTIVITY_TAG, "Caller try to find CODEC support "+"** "+CodecType+" **");

		int cnt = MediaCodecList.getCodecCount();
		Log.d(MediaCodecCore.ACTIVITY_TAG, "total support Codec number is: "+cnt+"");
		for (int i=0; i<cnt; i++)
		{
			MediaCodecInfo mediacodecinfo = MediaCodecList.getCodecInfoAt(i);

			boolean isEncoder = mediacodecinfo.isEncoder();
			if (isEncoder)
			{
				String codecname = mediacodecinfo.getName();
				if (codecname.indexOf("OMX.google") == -1 && codecname.indexOf("OMX.") != -1)
				{
					Log.d(MediaCodecCore.ACTIVITY_TAG, "Codec name is: "+codecname);
					String[] supportedtypes = mediacodecinfo.getSupportedTypes();
					if (supportedtypes == null ) {
						Log.d(MediaCodecCore.ACTIVITY_TAG, "do not have supported types! ");
						continue;
					}
					int typescnt = supportedtypes.length; Log.d(MediaCodecCore.ACTIVITY_TAG, codecname+"$ support media types num is: "+typescnt+"");

					for (int j=0; j<typescnt; j++)
					{
						MediaCodecInfo.CodecCapabilities codeccap = mediacodecinfo.getCapabilitiesForType(supportedtypes[j]);
						if (codeccap == null ) {
							Log.d(MediaCodecCore.ACTIVITY_TAG, "do not have CodecCapabilities by this type! ");
							continue;
						}
						
						Log.d(MediaCodecCore.ACTIVITY_TAG, "support media type : "+supportedtypes[j]); //list support media types of COCEC
						if (supportedtypes[j].indexOf(CodecType) != -1) {//CODEC match requirement
							int firstFormat = -1;
							Log.d(MediaCodecCore.ACTIVITY_TAG, "CODEC support found!!!");
							Log.d(MediaCodecCore.ACTIVITY_TAG, "(profile, level) = "+"("+codeccap.profileLevels[j].profile +""+", "+codeccap.profileLevels[j].level +""+")");
							int capcnt = codeccap.colorFormats.length; Log.d(MediaCodecCore.ACTIVITY_TAG, "total support color formats followed = "+capcnt+"");
							for (int k=0; k<capcnt; k++)
							{
								Log.d(MediaCodecCore.ACTIVITY_TAG, "color format = "+codeccap.colorFormats[k]+"");
                                if (codeccap.colorFormats[k] == src_format) {
                                    Log.d(MediaCodecCore.ACTIVITY_TAG, "input color format support, done!");
                                    return (int)(codeccap.colorFormats[k]);
                                }
								else if (k == 0) { //save the first format
									Log.d(MediaCodecCore.ACTIVITY_TAG, "save the first ColorFormat!");
									firstFormat = codeccap.colorFormats[0];
								}
							}
							Log.d(MediaCodecCore.ACTIVITY_TAG, "return the first color format, done!");
							return firstFormat;
						} else {
							Log.d(MediaCodecCore.ACTIVITY_TAG, "This CODEC does NOT support "+"** "+CodecType+" **");
						}
					}
				}
			} //isEncoder
		}

		return -1; //error case, no suitable CODEC found!
	}

	/* decode video frame start */
	public byte[] DecodeVideoFrame(MediaCodec codec, byte[] input, int flags) {
		codec_status = 0;
		do {
			try {
				if (input != null) {
					if (inputBuffers == null) {
						inputBuffers = codec.getInputBuffers();
					}
					int inputBufferIndex;
                    if (flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG) {
                        inputBufferIndex = codec.dequeueInputBuffer(-1); //infinite
                    }
                    else {
                        inputBufferIndex = codec.dequeueInputBuffer(100*1000); //100ms
                    }
					//Log.d(MediaCodecCore.ACTIVITY_TAG, "inputbuffer index = "+inputBufferIndex+"");
					if (inputBufferIndex >= 0) {
						ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
						//Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Video input buffer size = "+inputBuffer.limit()+"");
						inputBuffer.clear();
						inputBuffer.put(input);
						codec.queueInputBuffer(inputBufferIndex, 0, input.length, 0, flags);
					}
					input = null; //after use
				}
				
				if (outputBuffers == null || bufferInfo == null) {
					outputBuffers = codec.getOutputBuffers();
					bufferInfo = new MediaCodec.BufferInfo();
				}	
				
				int outputBufferIndex = codec.dequeueOutputBuffer(bufferInfo, 5*1000);
				//Log.d(MediaCodecCore.ACTIVITY_TAG, "outputbuffer index = "+outputBufferIndex+"");
				if (outputBufferIndex >= 0) {
					ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
                    //Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Check if Video bytebuffer is Direct: = "+outputBuffer.isDirect()+"");
					if (outSizeset == 0)
					{
						if (bufferInfo.offset != 0) crop_lt = bufferInfo.offset; //data offset
						outData = new byte[outputBuffer.limit()];
						Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Video output buffer size = "+outputBuffer.limit()+"");
						Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Video output data ..size = "+bufferInfo.size+"");
						outSizeset = 1;
					}
					outputBuffer.get(outData);
					outputBuffer.clear(); //must DO!!!!!!
					codec.releaseOutputBuffer(outputBufferIndex, false);
					
					codec_status |= 0x1; //0001
					//bufferInfo = null;
					return outData;
				}
				else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
					Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video output buffers changed ... *****");
					outputBuffers = codec.getOutputBuffers();
					codec_status |= 0x2; //0010
					outSizeset = 0;
					outData = null;
					//bufferInfo = null;
				}
				else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
					Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video media format changed ... *****");
					codec_status |= 0x4; //0100
					outSizeset = 0;
					outData = null;
					//bufferInfo = null;
				}
				else {
					return null;
				}
			}
			catch (Throwable t) {
				Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video meet problems, give up!!! *****");
				t.printStackTrace();
				codec_status = -1;
				return null;
			}
		} while(true);
		//return null;
	}
	/* decode video frame end */

	/* encode video frame start */
	public byte[] EncodeVideoFrame(MediaCodec codec, byte[] input, int flags) {
		codec_status = 0;
        crop_lt      = 0;
		do {
			try {
				if (input != null) {
					if (inputBuffers == null) {
						Log.d(MediaCodecCore.ACTIVITY_TAG, "get inputBuffers once ...");
						inputBuffers = codec.getInputBuffers();
                        Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Video input buffer limit = "+inputBuffers[0].limit()+"");
					}
					int inputBufferIndex = codec.dequeueInputBuffer(5*1000); //100ms
					if (inputBufferIndex >= 0) {
						ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
						inputBuffer.clear();
						inputBuffer.put(input);
						codec.queueInputBuffer(inputBufferIndex, 0, input.length, 0, flags);
					}
				}
				if (outputBuffers == null || bufferInfo == null) {
					outputBuffers	= codec.getOutputBuffers();
					bufferInfo		= new MediaCodec.BufferInfo();
					Log.i(MediaCodecCore.ACTIVITY_TAG, "get outputBuffers and bufferInfo ...");
				}
				int outputBufferIndex = codec.dequeueOutputBuffer(bufferInfo, 5*1000);
				if (outputBufferIndex >= 0) {
					ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
                    int sizeBuffer = outputBuffer.limit()/30;
					if (outSizeset == 0 || bufferInfo.size > sizeBuffer)
					{
                        if (bufferInfo.size > sizeBuffer)
                            sizeBuffer = bufferInfo.size;
						outData = new byte[sizeBuffer];
						Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Video output buffer limit = "+outputBuffer.limit()+"");
						outSizeset = 1;
					}
					outputBuffer.get(outData);
					outputBuffer.clear(); //must DO!!!!!!
					
					codec.releaseOutputBuffer(outputBufferIndex, false);
					codec_status |= 0x1; //0001
                    crop_lt = bufferInfo.size; //data size

					return outData;
				}
				else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
					Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video output buffers changed ... *****");
					outputBuffers = codec.getOutputBuffers();
					codec_status |= 0x2; //0010
					outSizeset = 0;
					outData = null;
					//bufferInfo = null;
                    input = null;
				}
				else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
					Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video media format changed ... *****");
					codec_status |= 0x4; //0100
					outSizeset = 0;
					outData = null;
					//bufferInfo = null;
                    input = null;
				}
				else {
					return null;
				}
			}
			catch (Throwable t) {
				Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video meet problems, give up!!! *****");
				t.printStackTrace();
				codec_status = -1;
				return null;
			}
		} while(true);
		//return null;
	}
	/* encode video frame end */

    /* set private audio codec instance */
    public void MediaSetMediaCodecInstance(MediaCodec codec) {
        mCodec = codec;
    }

	/* Audio Queue input buffer index */
	public int AudioCheckInputBufferAvail(int flags) {
		int usTimeOut = 1*10; //10us

        return ((int)mCodec.dequeueInputBuffer(usTimeOut));
	}

    /* Video Queue input buffer index */
	public int VideoCheckInputBufferAvail(int flags) {
		int usTimeOut = 30*1000; //30ms

        return ((int)mCodec.dequeueInputBuffer(usTimeOut));
	}
	
	/* Queue input data */
	public int MediaQueueDataToInput(byte[] input, int index, int flags) {
		ByteBuffer inputBuffer = inputBuffers[index];

        inputBuffer.clear();
        inputBuffer.put(input);
        mCodec.queueInputBuffer(index, 0, input.length, 0, flags);
        return 0;
	}

    /* Queue NULL */
	public int MediaQueueNullToInput(byte[] input, int index, int flags) {
        mCodec.queueInputBuffer(index, 0, 0, 0, flags);
        return 0;
	}

    /* Queue input buffer & data */
	public int AudioQueueInputBuffer(MediaCodec codec, byte[] input, int flags) {
		try {
			if (inputBuffers == null) {
				inputBuffers = codec.getInputBuffers();
			}
			int inputBufferIndex;
			if (flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG) {
				inputBufferIndex = codec.dequeueInputBuffer(-1); //infinite
			}
			else {
				inputBufferIndex = codec.dequeueInputBuffer(10*1000); //10ms
			}

			//Log.d(MediaCodecCore.ACTIVITY_TAG, "inputbuffer index = "+inputBufferIndex+"");
			if (inputBufferIndex >= 0) {
				ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
				//Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Audio input buffer size = "+inputBuffer.limit()+"");
				inputBuffer.clear();
				inputBuffer.put(input);
				//Log.d(MediaCodecCore.ACTIVITY_TAG, "inputbuffer length = "+input.length+"");
				codec.queueInputBuffer(inputBufferIndex, 0, input.length, 0, flags);
				input = null; //after use
			}
			return inputBufferIndex;
		}
		catch (Throwable t) {
			Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Audio meet problems, give up!!! *****");
			t.printStackTrace();
			codec_status = -1;
			return -1;
		}
	}

    /* Get input buffer size */
	public int MediaQueueInputBufferSize(byte[] input, int flags) {
		try {
            /* basic initialization */
			if (inputBuffers == null) {
				inputBuffers = mCodec.getInputBuffers();
				Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Media number of Input Buffers= "+inputBuffers.length+"");
			}
            if (outputBuffers == null || bufferInfo == null) {
                outputBuffers	= mCodec.getOutputBuffers();
                bufferInfo		= new MediaCodec.BufferInfo();
                Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Media number of Output Buffers= "+outputBuffers.length+"");
            }

			int inputBufferIndex;
			if (flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG) {
				inputBufferIndex = mCodec.dequeueInputBuffer(-1); //infinite
			}
			else {
				inputBufferIndex = mCodec.dequeueInputBuffer(10*1000); //10ms
			}

			//Log.d(MediaCodecCore.ACTIVITY_TAG, "inputbuffer index = "+inputBufferIndex+"");
			if (inputBufferIndex >= 0) {
				ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
				//Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Audio input buffer size = "+inputBuffer.limit()+"");
				inputBuffer.clear();
				inputBuffer.put(input);
				//Log.d(MediaCodecCore.ACTIVITY_TAG, "inputbuffer length = "+input.length+"");
				mCodec.queueInputBuffer(inputBufferIndex, 0, input.length, 0, flags);
				input = null; //after use
				return ((int)inputBuffer.limit());
			}
			return -1;
		}
		catch (Throwable t) {
			Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Audio meet problems, give up!!! *****");
			t.printStackTrace();
			codec_status = -1;
			return -1;
		}
	}

	/* Audio Dequeue output buffer */
	public ByteBuffer AudioDeQueueOutputBuffer() {
		codec_status = 0;
		do {
            g_outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 1*10); /* 10us */
            if (g_outputBufferIndex >= 0) {
                ByteBuffer outputBuffer = outputBuffers[g_outputBufferIndex];

                crop_lt = bufferInfo.size;
                return outputBuffer;
            }
            else if (g_outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Audio output buffers changed ... *****");
                outputBuffers = mCodec.getOutputBuffers();
                codec_status |= 0x2; //0010
            }
            else if (g_outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Audio media format changed ... *****");
                codec_status |= 0x4; //0100
            }
            else {
                return null;
            }
		} while(true);
	}

    /* Video Dequeue output buffer */
	public ByteBuffer VideoDeQueueOutputBuffer() {
		codec_status = 0;
		do {
            g_outputBufferIndex = mCodec.dequeueOutputBuffer(bufferInfo, 1*1000); /* 1000us */
            Log.d(MediaCodecCore.ACTIVITY_TAG, "output index = "+g_outputBufferIndex+"");
            if (g_outputBufferIndex >= 0) {
                ByteBuffer outputBuffer = outputBuffers[g_outputBufferIndex];

                crop_lt = bufferInfo.offset;
                return outputBuffer;
            }
            else if (g_outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
                Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video output buffers changed ... *****");
                outputBuffers = mCodec.getOutputBuffers();
                codec_status |= 0x2; //0010
            }
            else if (g_outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
                Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Video media format changed ... *****");
                codec_status |= 0x4; //0100
            }
            else {
                return null;
            }
		} while(true);
	}

    /* Release output buffer */
    public void MediaReleaseOutputBuffer() {
        //ByteBuffer outputBuffer = outputBuffers[g_outputBufferIndex];
        //outputBuffer.clear(); //must DO!!!!!!
        mCodec.releaseOutputBuffer(g_outputBufferIndex, false);
    }

	/* glabal variables */
	public byte[] DecodeAudioFrame(MediaCodec codec, byte[] input, int flags) {
		int timeout_us = 100*1; //get input buffer anyway
		codec_status = 0;
		do {
			try {
				if (input != null) {
					if (inputBuffers == null) {
						inputBuffers = codec.getInputBuffers();
					}
					int inputBufferIndex;
                    if (flags == MediaCodec.BUFFER_FLAG_CODEC_CONFIG) {
                        inputBufferIndex = codec.dequeueInputBuffer(-1); //infinite
                    }
                    else {
                        inputBufferIndex = codec.dequeueInputBuffer(timeout_us); //100ms
                    }
					//Log.d(MediaCodecCore.ACTIVITY_TAG, "inputbuffer index = "+inputBufferIndex+"");
					if (inputBufferIndex >= 0) {
						ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
						inputBuffer.clear();
						inputBuffer.put(input);
						codec.queueInputBuffer(inputBufferIndex, 0, input.length, 0, flags);
					}
					input = null; //after use
				}

				if (outputBuffers == null || bufferInfo == null) {
					outputBuffers = codec.getOutputBuffers();
					bufferInfo = new MediaCodec.BufferInfo();
				}
				/* check if there are output buffers should send out first */
				int outputBufferIndex;
				if (flags == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
					outputBufferIndex = codec.dequeueOutputBuffer(bufferInfo, 100*1000); /* 100ms */
				} else
					outputBufferIndex = codec.dequeueOutputBuffer(bufferInfo, 1*100); /* 100us */
				//Log.d(MediaCodecCore.ACTIVITY_TAG, "outputbuffer index = "+outputBufferIndex+"");
				if (outputBufferIndex >= 0) {
					ByteBuffer outputBuffer = outputBuffers[outputBufferIndex];
					if (outSizeset == 0)
					{
						if (bufferInfo.offset != 0) crop_lt = bufferInfo.offset; //data offset
						Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Audio output buffer size = "+outputBuffer.limit()+"");
						Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@Audio output data ..size = "+bufferInfo.size+"");
						outData = new byte[bufferInfo.size];
						outSizeset = 1;
					}
					//boolean hasArray = outputBuffer.hasArray();
					//Log.i(MediaCodecCore.ACTIVITY_TAG, "************ @@if ByteBuffer has array: "+hasArray+"");
					outputBuffer.get(outData);
					outputBuffer.clear(); //must DO!!!!!!
					codec.releaseOutputBuffer(outputBufferIndex, false);
					
					codec_status |= 0x1; //0001
					return outData;
				}
				else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_BUFFERS_CHANGED) {
					Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Audio output buffers changed ... *****");
					outputBuffers = codec.getOutputBuffers();
					codec_status |= 0x2; //0010
					outSizeset = 0;
					outData = null;
					//bufferInfo = null;
				}
				else if (outputBufferIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED) {
					Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Audio media format changed ... *****");
					codec_status |= 0x4; //0100
					outSizeset = 0;
					outData = null;
					//bufferInfo = null;
				}
				else {
					return null;
				}
			}
			catch (Throwable t) {
				Log.i(MediaCodecCore.ACTIVITY_TAG, "************ Notice: @@Audio meet problems, give up!!! *****");
				t.printStackTrace();
				codec_status = -1;
				return null;
			}
		} while(true);
		//return null;
	}
	/* decode audio frame end */
	
	public void flush(MediaCodec codec) {
		try {
			Log.i(MediaCodecCore.ACTIVITY_TAG, " Call MediaCodec flush ...");
			codec.flush();
		} catch (Exception e){ 
			e.printStackTrace();
		}
	}
	
	public void close(MediaCodec codec) {
		try {
			Log.i(MediaCodecCore.ACTIVITY_TAG, " Call MediaCodec close and release ...");
			outData = null;
			codec.stop();
			codec.release();
			codec = null;
		} catch (Exception e){ 
			e.printStackTrace();
		}
		
	}

}
