export const Error = {
	NO_ERROR:            0x0, // Graceful shutdown
	PROTOCOL_ERROR:      0x1, // Protocol error detected
	INTERNAL_ERROR:      0x2, // Implementation fault
	FLOW_CONTROL_ERROR:  0x3, // Flow-control limits exceeded
	SETTINGS_TIMEOUT:    0x4, // Settings not acknowledged
	STREAM_CLOSED:       0x5, // Frame received for closed stream
	FRAME_SIZE_ERROR:    0x6, // Frame size incorrect
	REFUSED_STREAM:      0x7, // Stream not processed
	CANCEL:              0x8, // Stream cancelled
	COMPRESSION_ERROR:   0x9, // Compression state not updated
	CONNECT_ERROR:       0xa, // TCP connection error for CONNECT method
	ENHANCE_YOUR_CALM:   0xb, // Processing capacity exceeded
	INADEQUATE_SECURITY: 0xc, // Negotiated TLS parameters not acceptable
	HTTP_1_1_REQUIRED:   0xd, // Use HTTP/1.1 for the request
};

export class HTTP2Exception {

};

export default Error;
