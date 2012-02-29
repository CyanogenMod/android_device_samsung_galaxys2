/******************************************************************************
 * GPS HAL wrapper
 * wrapps around Samsung GPS Libary and replaces a faulty pointer to
 * a faulty function from Samsung that will cause the system_server
 * to crash.
 *
 * Copyright 2010 - Kolja Dummann
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include <hardware/hardware.h>
#include <hardware/gps.h>
#include <errno.h>
#include <dlfcn.h>

//#define LOG_NDEBUG 0

#include <stdlib.h>
#define LOG_TAG "gps-wrapper"
#include <utils/Log.h>

#define ORIGINAL_HAL_PATH "/system/lib/hw/vendor-gps.exynos4.so"

static const AGpsRilInterface* oldAGPSRIL = NULL;
static AGpsRilInterface newAGPSRIL;

static const GpsInterface* originalGpsInterface = NULL;
static GpsInterface newGpsInterface;

/**
 * Load the file defined by the variant and if successful
 * return the dlopen handle and the hmi.
 * @return 0 = success, !0 = failure.
 */
static int load(const char *id,
        const char *path,
        const struct hw_module_t **pHmi)
{
    int status;
    void *handle;
    struct hw_module_t *hmi;

    /*
     * load the symbols resolving undefined symbols before
     * dlopen returns. Since RTLD_GLOBAL is not or'd in with
     * RTLD_NOW the external symbols will not be global
     */
    handle = dlopen(path, RTLD_NOW);
    if (handle == NULL) {
        char const *err_str = dlerror();
        LOGE("load: module=%s\n%s", path, err_str?err_str:"unknown");
        status = -EINVAL;
        goto done;
    }

    /* Get the address of the struct hal_module_info. */
    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    hmi = (struct hw_module_t *)dlsym(handle, sym);
    if (hmi == NULL) {
        LOGE("load: couldn't find symbol %s", sym);
        status = -EINVAL;
        goto done;
    }

    /* Check that the id matches */
    if (strcmp(id, hmi->id) != 0) {
        LOGE("load: id=%s != hmi->id=%s", id, hmi->id);
        status = -EINVAL;
        goto done;
    }

    hmi->dso = handle;

    /* success */
    status = 0;

    done:
    if (status != 0) {
        hmi = NULL;
        if (handle != NULL) {
            dlclose(handle);
            handle = NULL;
        }
    } else {
        LOGV("loaded HAL id=%s path=%s hmi=%p handle=%p",
                id, path, *pHmi, handle);
    }

    *pHmi = hmi;

    return status;
}

static void update_network_state_wrapper(int connected, int type, int roaming, const char* extra_info)
{
    LOGI("%s was called and saved your from a faulty implementation ;-)", __func__);
}

static const void* wrapper_get_extension(const char* name)
{
    LOGV("%s was called", __func__);
    
    if (!strcmp(name, AGPS_RIL_INTERFACE) && (oldAGPSRIL = originalGpsInterface->get_extension(name)))
    {
        LOGV("%s AGPS_RIL_INTERFACE extension requested", __func__);
        /* use a wrapper to avoid calling samsungs faulty implemetation */        
        newAGPSRIL.size = sizeof(AGpsRilInterface);
        newAGPSRIL.init = oldAGPSRIL->init;
        newAGPSRIL.set_ref_location = oldAGPSRIL->set_ref_location;
        newAGPSRIL.set_set_id = oldAGPSRIL->set_set_id;
        newAGPSRIL.ni_message = oldAGPSRIL->ni_message;
        LOGV("%s setting update_network_state_wrapper", __func__);
        newAGPSRIL.update_network_state = update_network_state_wrapper;
        return &newAGPSRIL;
    }
    return originalGpsInterface->get_extension(name);
}

/* HAL Methods */
const GpsInterface* gps_get_gps_interface(struct gps_device_t* dev)
{
    hw_module_t* module;
    int err;
    
	LOGV("%s was called", __func__);    
    
    err = load(GPS_HARDWARE_MODULE_ID, ORIGINAL_HAL_PATH, (hw_module_t const**)&module);
        
    if (err == 0) {
        LOGV("%s vendor lib loaded", __func__);
        hw_device_t* device;
        struct gps_device_t  *gps_device;
        err = module->methods->open(module, GPS_HARDWARE_MODULE_ID, &device);
        if (err == 0) {
            LOGV("%s got gps device", __func__);
            gps_device = (struct gps_device_t *)device;            
            originalGpsInterface = gps_device->get_gps_interface(gps_device);            
            LOGV("%s  device set", __func__);
        }
    }    

    if(originalGpsInterface)
    {
        LOGV("%s exposing callbacks", __func__); 
        newGpsInterface.size = sizeof(GpsInterface);
        newGpsInterface.init = originalGpsInterface->init;
        newGpsInterface.start = originalGpsInterface->start;
        newGpsInterface.stop = originalGpsInterface->stop;
        newGpsInterface.cleanup = originalGpsInterface->cleanup;
        newGpsInterface.inject_time = originalGpsInterface->inject_time;
        newGpsInterface.inject_location = originalGpsInterface->inject_location;
        newGpsInterface.delete_aiding_data = originalGpsInterface->delete_aiding_data;
        newGpsInterface.set_position_mode = originalGpsInterface->set_position_mode;
        LOGV("%s setting extension wrapper", __func__);
        newGpsInterface.get_extension = wrapper_get_extension;

    }
    LOGV("%s done", __func__);
    return &newGpsInterface;
}

static int open_gps(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    struct gps_device_t *dev = malloc(sizeof(struct gps_device_t));
    memset(dev, 0, sizeof(*dev));

    LOGV("%s was called", __func__);

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->get_gps_interface = gps_get_gps_interface;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t gps_module_methods = {
    .open = open_gps
};

const struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "GPS HAL Wrapper Module",
    .author = "Kolja Dummann",
    .methods = &gps_module_methods,
};
