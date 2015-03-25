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
#ifndef __KPRHTTPSERVER__
#define __KPRHTTPSERVER__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "FskNetUtils.h"

#ifdef __KPRHTTPSERVER_PRIV__
#define kKprHTTPServerDefaultCretificates "\
-----BEGIN CERTIFICATE-----\n\
MIIDzDCCArSgAwIBAgIBATANBgkqhkiG9w0BAQUFADA9MQswCQYDVQQGEwJVUzEL\n\
MAkGA1UECBMCQ0ExEDAOBgNVBAoTB01hcnZlbGwxDzANBgNVBAsTBktpbm9tYTAe\n\
Fw0xNTAzMTMyMjM1MzJaFw0yNTAzMTAyMjM1MzJaMEsxCzAJBgNVBAYTAlVTMQsw\n\
CQYDVQQIEwJDQTEQMA4GA1UEChMHTWFydmVsbDEPMA0GA1UECxMGS2lub21hMQww\n\
CgYDVQQDEwNLUFIwggEgMA0GCSqGSIb3DQEBAQUAA4IBDQAwggEIAoIBAQCoLdf5\n\
kuibvyEoVvEuLCoJ7r8DjeWixvBrZQ3JevVPl/lNBsgZeoehkOHJyrEiA9t7+jbg\n\
6+8B6UxlkpI3qsLEVZetLwh7AT6IuiBloZVEzwrqG3UYPoov9v+NYgeXoXEG+P8a\n\
Wm5BDpKsaBR6a6uyKaBrutleZgAYXP5GZzz97JXBDKde2LFey5wA4E8plRG6buMS\n\
78UMb9P2sTDs2RvccYcCLgk34RfD45vNfYNZ63ZmUETFH5UaZn4j31TjS8eUJIId\n\
bK3j7326tiECQZjB9aFNx8WFGRP0fvhMUNhNmzKtl8MAiXov0GqIerkQsjUBeix2\n\
/iMuZ3wdaO5FSxCzAgEDo4HKMIHHMAkGA1UdEwQCMAAwLAYJYIZIAYb4QgENBB8W\n\
HU9wZW5TU0wgR2VuZXJhdGVkIENlcnRpZmljYXRlMB0GA1UdDgQWBBTdZFySS994\n\
vim+o6S5T64d6CvCTTBtBgNVHSMEZjBkgBRBd93PahOqZR0WU6E7V/AVKqaSjKFB\n\
pD8wPTELMAkGA1UEBhMCVVMxCzAJBgNVBAgTAkNBMRAwDgYDVQQKEwdNYXJ2ZWxs\n\
MQ8wDQYDVQQLEwZLaW5vbWGCCQC7gG4ePPerZTANBgkqhkiG9w0BAQUFAAOCAQEA\n\
GwanPo07b4jBt66bOON+HaQV2zf3U4h1+kimrIafIqNIZ8hm6arKiRO6xRl9v7Rd\n\
PvaxT58pnxY+g7dPe0duSsSV1OJ9r1rLBT1A9Q7joJqn1Au/z/71EQoTM/qArxJx\n\
ic0KjbtU1BlECt1x0GiIQ87NQCjwoyUxqUJUvudsRT56uTLW8tOCTtqzG7koILsc\n\
zsVfz/txsnk05yoO4f1URkx0lOqx7cvD5Q7R3zwEQSQayb86p7hZdQ5oJbROVc0u\n\
9yVqVm69lUouy1KckeVvaxN/if+rt280cUdEDJHD85MF9UtIL3rSstbRrOfD26jN\n\
rpujr0Pu90EfaUItjql/yw==\n\
-----END CERTIFICATE-----\n\
"

#define kKprHTTPServerDefaultKey "\
-----BEGIN RSA PRIVATE KEY-----\n\
MIIEowIBAAKCAQEAqC3X+ZLom78hKFbxLiwqCe6/A43losbwa2UNyXr1T5f5TQbI\n\
GXqHoZDhycqxIgPbe/o24OvvAelMZZKSN6rCxFWXrS8IewE+iLogZaGVRM8K6ht1\n\
GD6KL/b/jWIHl6FxBvj/GlpuQQ6SrGgUemursimga7rZXmYAGFz+Rmc8/eyVwQyn\n\
XtixXsucAOBPKZURum7jEu/FDG/T9rEw7Nkb3HGHAi4JN+EXw+ObzX2DWet2ZlBE\n\
xR+VGmZ+I99U40vHlCSCHWyt4+99urYhAkGYwfWhTcfFhRkT9H74TFDYTZsyrZfD\n\
AIl6L9BqiHq5ELI1AXosdv4jLmd8HWjuRUsQswIBAwKCAQBwHo/7t0W9KhYa5KDJ\n\
csaxSdStCUPB2fWc7gkw/KOKZVDeBIVmUa/BC0ExMctsApJSps9AnUoBRjLuYbbP\n\
xyyC47pzdLBSANRbJsBDwQ4t31ycEk4QKbF1T1UI7AUPwPYEpf9m5vQrXwxy8A2m\n\
8nJ2xmryfJDpmVVlk1Qu733+nKPPBBZFpyjRu29N4GkShFl8Pj+cmNa+96wsnLBL\n\
TWO9C6qs2xdc1vvK3ILp+XOPF9jGrWnVy4XedGYZgVYPCnRNufmUGM63xulUcDzn\n\
3+ng14PWCZm3AGHZ+4xre7zIRbrjTZWZgm1QNi0COfX+6B6C/V9wmJf0okuVAEJl\n\
KGM7AoGBAN7PMlJ68AHusZdaQ/e5otK1r1G+ziDkvSJpA1c2bJ9gEqAnbh0Rgvze\n\
yxaKfC348p9RgY+N8eJeYj5BpS1Y50WS9bjNYtnY2mETCEkDJvuvaGBOgVn/ciOc\n\
0av2JGLhLliBtA/QyOxH+WXZ4Lm8rstDluSpbHYlnFCu8ntMO0zFAoGBAME7VDN7\n\
bfI1gN2xy7nUK7jKYjHpPGIJPy9KwtFFeGbgzWbXd0kbG6E0zcDkCygKVRHqypZ2\n\
fGnuVaa7+CZz1NOMl9Lx5J33ar6pBXyjSr5BSfs+OATzJl2Qs/m08tpABEHW71LJ\n\
fOnvhcErQw5Vpzv5ZzikrMkZ17oO9g9hUy8XAoGBAJSKIYxR9VafIQ+RgqUmbIx5\n\
H4vUiWtDKMGbV495nb+VYcAaSWi2V1M/Mg8G/XP7TGo2Vl+z9pbpltQrw3OQmi5h\n\
+SXeQeaQkZYMsDCsxKfKRZWJq5FU9sJoi8f5bZdAyZBWeAqLMJ2FUO6RQHvTHzIt\n\
D0MbnaQZEuB0oaeIJ4iDAoGBAIDSOCJSSUwjqz52h9E4HSXcQXabfZaw1Mox1zYu\n\
UESV3kSPpNtnZ8DN3oCYB3AG42FHMbmkUvFJjm8n+sRNOI0IZTdL7b6k8dRwrlMX\n\
hymA2/zUJViiGZO1zVEjTJGAAtaPSjcw/fFKWStyLLQ5Gif7miXDHdtmj9FfTrTr\n\
jMoPAoGBAM7BGJJ7iN4NZAAf3+CHSYSXd0Uli3C1mU6R57nm3WdGhRsNy3ttQh2s\n\
NFu8aKHeVsd67exM6lFEK5oETfXUr8juvTRuCEPV6XcYM9qMnZ0q2pFIDOJ8qCdf\n\
qoo4KHJ7q8v+gvDoTrjitM67T068txPkJzr4ua64PZPw8XYM/IJn\n\
-----END RSA PRIVATE KEY-----\n\
"
#endif

FskErr KprHTTPServerNew(KprHTTPServer* it, char* authority, char* path, UInt32 preferredPort, FskSocketCertificateRecord* certs);
FskAPI(void) KprHTTPServerDispose(KprHTTPServer self);
FskAPI(KprHTTPServer) KprHTTPServerGet(char* id);
FskAPI(UInt32) KprHTTPServerGetPort(KprHTTPServer self);
FskAPI(UInt32) KprHTTPServerGetTimeout(KprHTTPServer self);
FskAPI(Boolean) KprHTTPServerIsSecure(KprHTTPServer self);
FskAPI(void) KprHTTPServerSetTimeout(KprHTTPServer self, UInt32 timeout);
FskAPI(FskErr) KprHTTPServerStart(KprHTTPServer self);
FskAPI(FskErr) KprHTTPServerStop(KprHTTPServer self, Boolean flush);

FskAPI(void) KprHTTPTargetMessageSetResponseProtocol(KprMessage message, char* protocol);

FskAPI(void) KprNetworkInterfaceActivate(Boolean activateIt);
FskAPI(void) KprNetworkInterfaceCleanup();
FskAPI(void) KprNetworkInterfaceSetup();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
