#include <stdio.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <utils/Log.h>

using namespace android;

int main() {
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;

    do {
        binder = sm->getService(String16("TvoutService_C"));
        if (binder != 0) break;
        usleep(500000); // 0.5 s
    } while(true);

    int ret;

    Parcel s2, r2;
    s2.writeInterfaceToken(String16("android.hardware.ITvoutService"));
    binder->transact(1, s2, &r2);
    sp<IBinder> binder2 = r2.readStrongBinder();

    while (true) {
        {

            Parcel send, reply;
            int code = 4;
            send.writeInterfaceToken(String16("android.hardware.Tvout"));
            int ret = binder2->transact(code, send, &reply);
        }
        {

            Parcel send, reply;
            int code = 27;
            send.writeInterfaceToken(String16("android.hardware.ITvout"));
            int ret = binder2->transact(code, send, &reply);
        }
        {

            Parcel send, reply;
            int code = 13;
            send.writeInterfaceToken(String16("android.hardware.ITvout"));
            send.writeInt32(0);
            int ret = binder2->transact(code, send, &reply);
        }
        usleep(15000); // Should give ~60 fps
    }
    return 0;
}
