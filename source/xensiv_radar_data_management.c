/*****************************************************************************
 * File name: xensiv_radar_data_management.c
 *
 * Description: This file implements a data buffering scheme 
 * for radar data.
 *
*******************************************************************************
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/


#include <string.h>

#include "xensiv_radar_data_management.h"

/* ARM compiler also defines __GNUC__ */
#if defined (__GNUC__) && !defined(__ARMCC_VERSION)
#include <malloc.h>
#else
#include <stdlib.h>
#endif /* #if defined (__GNUC__) && !defined(__ARMCC_VERSION) */
#define FREERTOS_AWARE

//////////////////////////////////////////////////DECLARATION/////////////////////////////////////////////
#ifdef FREERTOS_AWARE

/*
 *\def typedef struct  subscribed_task_lists_s
 *
 * Attributes pertaining to every subscriber task
 */
typedef struct {

    bool data_read; /*<<indicate and mark if task has read the data*/

    TaskHandle_t suscriber_task_handle; /*<<The FREERTOS Task handle representing subscriber task*/

}subscribers_task_lists_s;

#endif


/*
 *\def typedef struct  manager_state_s
 *
 * Attributes for managing radar data.
 */
typedef struct {

    uint8_t *buffer; /*<< Pointer to heap for FIFO buffer allocation*/

    uint32_t buff_size; /*<< Total size of buffer in bytes FIFO buffer */

    uint32_t samples; /*<< Total number of bytes in FIFO */

    uint32_t head,tail; /*<< front and back positions of the FIFO queue*/

    uint32_t fill_level; /*<< FIFO water mark level in bytes*/

    uint8_t subscribers; /*<< Number of subscribers (task/callers)*/

#ifdef FREERTOS_AWARE
    subscribers_task_lists_s subscriptions[ACTIVE_SUBSCRIPTION_UB + 1]; /*<<list of all subscriber tasks of type \ref subscribers_task_lists_s*/
#else
    cb_radar_data_event subscriptions[ACTIVE_SUBSCRIPTION_UB + 1]; /*<<list of all subscriber tasks of type \ref cb_radar_data_event*/
#endif

    void* (*malloc_func)(size_t size); /*<<Hold reference to consumer supplied memory allocation*/

    void (* free_func)(void* ptr); /*<Hold reference to consumer supplied definition for releasing allocated memory<*/

}manager_state_s;


//////////////////////////////////////////////////DEFINITIONS //////////////////////////////////////////////////////
/*Instance of Internal state of radar data manager*/
static manager_state_s manager;

/*Local copy of radar data manager interface instance.
 * This is instantiated by owner of the RDM initialization
 * and supplied during radar_data_manager_init
 * */
static radar_data_manager_s *manager_interface;


//////////////////////////////////////////////////FUNCTIONAL DEFINITIONS/////////////////////////////////////////////

/*
 * subscribe to radar data
 */
#ifdef FREERTOS_AWARE
int32_t
radar_data_manager_subscribe(TaskHandle_t subscriber_task)
#else
int32_t
radar_data_manager_subscribe(cb_radar_data_event cb)
#endif
{
    //First check the sanity of parameter
    #ifdef FREERTOS_AWARE
    if (NULL == subscriber_task)
    {
        return -1;
    }
    #else
    if (NULL == cb)
    {
        return -1;
    }
    #endif
    //then check the already existing subscriptions
    for (uint8_t subs = 1; subs <= ACTIVE_SUBSCRIPTION_UB; subs++)
    {
        #ifdef FREERTOS_AWARE
        if (manager.subscriptions[subs].suscriber_task_handle == subscriber_task)
        {
            return subs;
        }
        #else
        if (manager.subscriptions[subs] == cb)
        {
            return subs;
        }
        #endif
    }
    //check if active subscriptions limit is reached or buffer in not initialized/RDM deinit etc.
    if ((manager.subscribers == ACTIVE_SUBSCRIPTION_UB) || (NULL == manager.buffer))
    {
        // Ran out of available subscriptions
        return -2;
    }

    for (uint8_t subs = 1; subs <= ACTIVE_SUBSCRIPTION_UB; subs++)
    {
        #ifdef FREERTOS_AWARE
        if (manager.subscriptions[subs].suscriber_task_handle == subscriber_task)
        {
            return subs;
        }

        if (NULL == manager.subscriptions[subs].suscriber_task_handle)
        {
            manager.subscriptions[subs].data_read = false;

            manager.subscriptions[subs].suscriber_task_handle = subscriber_task;

            manager.subscribers++;

            return subs;
        }
        #else /* ifdef FREERTOS_AWARE */

        if (NULL == manager.subscriptions[subs])
        {
            manager.subscriptions[subs]= cb;

            manager.subscribers++;

            return subs;
        }
        #endif /* ifdef FREERTOS_AWARE */
    }

    //indicate failure in case none of the above conditions met
    return -2;
}


/*
 * un-subscribe to radar data
 */
void
radar_data_manager_unsubscribe (int32_t subscription_id)
{
    if ((subscription_id <= 0) || (subscription_id > ACTIVE_SUBSCRIPTION_UB) || (manager.subscribers == 0))
    {
        return;
    }

#ifdef FREERTOS_AWARE
    manager.subscriptions[subscription_id].data_read = false;
    manager.subscriptions[subscription_id].suscriber_task_handle = NULL;
#else

    manager.subscriptions[subscription_id]= NULL;
#endif

    manager.subscribers--;
}


/*
 * trigger radar data manager
 */
#ifdef FREERTOS_AWARE
void
radar_data_manager_run(bool run_from_isr)
#else
void
radar_data_manager_run()
#endif
{
    uint32_t samples;

    if (manager.tail < manager.buff_size)
    {

        int32_t result = manager_interface->in_read_radar_data((void*)(manager.buffer + manager.tail), &samples,
                (manager.buff_size - manager.tail));

        if (result >= 0)
        {
            if ( samples <= (manager.buff_size - manager.tail))
            {
                //This implies a successful read
                manager.tail += samples;
                manager.samples = manager.tail - manager.head;
            }
            else
            {
                //handle anomaly
                //anomaly includes failure to read data
                //read data size is more than acceptable UB set by RDM etc.
            }

        }

    }

    //now check if the fill level is attained
    if (manager.samples >= manager.fill_level)
    {
#ifdef FREERTOS_AWARE

        //now inform all subscribers about available data
        for (int sub = 1; sub <= ACTIVE_SUBSCRIPTION_UB; sub++)
        {
            if (NULL != manager.subscriptions[sub].suscriber_task_handle)
            {

                if (run_from_isr)
                {
                    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

                    vTaskNotifyGiveFromISR(manager.subscriptions[sub].suscriber_task_handle, &xHigherPriorityTaskWoken);

                    /* Context switch needed? */
                    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
                }
                else
                {
                    xTaskNotifyGive(manager.subscriptions[sub].suscriber_task_handle);
                }

            }
        }

        // now adjust queue if all the subscribers tasks have consumed the data
        bool adjust_queue = true;
        for (int sub = 1; sub <= ACTIVE_SUBSCRIPTION_UB; sub++)
        {
            if ((NULL != manager.subscriptions[sub].suscriber_task_handle) &&
                (manager.subscriptions[sub].data_read == false))
            {
                adjust_queue = false;
            }
        }

        if (adjust_queue == true)
        {
            // now adjust the queue by fill level
            // considering reader has read all data till fill level
            manager.head += manager.fill_level;

            //move the buffer to front by amount data already read
            if (manager.head >0)
            {
                uint32_t sz = (manager.tail - manager.head);

                //move the buffer for new data
                memcpy(manager.buffer, (manager.buffer + manager.head), sz);

                manager.tail = sz;

                manager.head = 0;


            }
            for (int sub = 1; sub <= ACTIVE_SUBSCRIPTION_UB; sub++)
            {

                if (manager.subscriptions[sub].data_read == true )
                {
                    manager.subscriptions[sub].data_read = false;
                }
            }

        }

#else
        //now inform all subscribers about available data
        for (int sub = 1; sub <= ACTIVE_SUBSCRIPTION_UB; sub++)
        {
            if (NULL != manager.subscriptions[sub])
            {
                manager.subscriptions[sub](manager.buffer + manager.head, manager.fill_level);
            }
        }

        // now adjust the queue
        manager.head += manager.fill_level;

        //move the buffer to front by amount data already read
        if (manager.head >0)
        {
            uint32_t sz = (manager.tail - manager.head);

            //move the buffer for new data
            memcpy(manager.buffer, (manager.buffer + manager.head), sz);

            manager.tail = sz;

            manager.head = 0;


        }



#endif

    }
    //finally validate the head and tail of queue
    if ((manager.tail == manager.buff_size) && (manager.head == manager.tail))
    {
        manager.tail = 0;
        manager.head = 0;
        manager.samples = 0;

        //reset the buffer
        memset((void*)manager.buffer,0,manager.buff_size);

    }

}


#ifdef FREERTOS_AWARE

/*
 * read from RDM data buffer
 */
int32_t
radar_data_manager_read_buffer(int32_t subscription_id, uint16_t **data_ptr, uint32_t *size)
{
    if ((subscription_id <= 0) || (subscription_id > ACTIVE_SUBSCRIPTION_UB))
    {
        return -1;
    }

    if (manager.samples < manager.fill_level)
    {
        return -2;
    }

    if (NULL != manager.subscriptions[subscription_id].suscriber_task_handle)
    {
        *data_ptr = (uint16_t*) (manager.buffer + manager.head);

        *size = (manager.fill_level);

    }
    else
    {
        return -2;
    }

    return 0;
}

/*
 * acknowledge the data read
 */
void
radar_data_manager_ack_data_read(int32_t subscription_id)
{
    if ((subscription_id <= 0) || (subscription_id > ACTIVE_SUBSCRIPTION_UB))
    {
        return;
    }

    manager.subscriptions[subscription_id].data_read = true;

}

#endif


/*
 * set RDM buffer fill level
 */
int32_t radar_data_manager_set_fill_level(int32_t fill_level)
{
    if ((0 == fill_level) ||
        (fill_level > manager.buff_size))
    {
        return -1;
    }

    manager.fill_level = fill_level;

    return 0;

}

/*
 * get RDM buffer fill level
 */
int32_t radar_data_manager_get_fill_level(void)
{
    return manager.fill_level;
}

/*
 * set platform specific malloc and free
 */
void radar_data_manager_set_malloc_free(void* (*malloc_func)(size_t size),
        void (* free_func)(void* ptr))
{

    manager.malloc_func = malloc_func;

    manager.free_func =  free_func;

}


/*
 * Initialize the RDM
 */
int32_t
radar_data_manager_init(radar_data_manager_s* mgr_interface, uint32_t buffer_size, uint32_t fill_level)
{
    //first check if RDM is already initialized
    if (NULL != manager.buffer)
    {
        return -2;
    }

    if ((NULL == mgr_interface) || (0 == buffer_size) || (0 == fill_level) ||
        (fill_level > buffer_size))
    {
        return -1;
    }

    //Allocate buffer.
    if ((NULL == manager.malloc_func) || (NULL == manager.free_func))
    {
        manager.malloc_func = malloc;
        manager.free_func =  free;
    }

    manager.buffer = (uint8_t*) manager.malloc_func(buffer_size);

    if (NULL == manager.buffer)
    {
        return -2;
    }

    //reset the buffer
    memset((void*)manager.buffer,0,buffer_size);

    manager.fill_level = fill_level;

    manager.buff_size = buffer_size;

    manager.samples = 0;

    manager.subscribers = 0;

    manager.head = 0;
    manager.tail = 0;

    mgr_interface->subscribe = radar_data_manager_subscribe;

    mgr_interface->unsubscribe = radar_data_manager_unsubscribe;

    mgr_interface->run = radar_data_manager_run;

    mgr_interface->set_fill_level = radar_data_manager_set_fill_level;

    mgr_interface->get_fill_level = radar_data_manager_get_fill_level;

#ifdef FREERTOS_AWARE
    mgr_interface->read_from_buffer = radar_data_manager_read_buffer;

    mgr_interface->ack_data_read = radar_data_manager_ack_data_read;
#endif

    manager_interface = mgr_interface;

    return 0;
}


/*
 * Free RDM
 */
int32_t radar_data_manager_deinit()
{

    //make sure no active subscriptions exist
    if ((manager.subscribers > 0) || (manager.buffer == NULL))
    {
        return -2;
    }

    manager.free_func(manager.buffer);

    memset(&manager, 0, sizeof(manager_state_s));
    return 0;
}




