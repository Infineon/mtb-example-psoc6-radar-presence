
#ifndef XENSIV_RADAR_DATA_MANAGEMENT_H_
#define XENSIV_RADAR_DATA_MANAGEMENT_H_

#ifdef CY_RTOS_AWARE
#define FREERTOS_AWARE
#endif

#include <stdint.h>
#include <stddef.h>
#ifdef FREERTOS_AWARE
#include "FreeRTOS.h"
#include "task.h"
#endif

/*
 * @def ACTIVE_SUBSCRIPTION_UB
 * Maximum supported active subscriptions
 * @note: This value can be change to larger number depending on required
 *        active subscription
 */
#define ACTIVE_SUBSCRIPTION_UB 4


/*
 * @def enum radar_data_manager_err_codes_e
 * Possible return codes for every interface and functions
 * Caller can use these codes by casting the integer return values of all 
 * interfaces and functions of RDM
 * Also for expected interfaces, caller shall implement return codes as defined here
 */
typedef enum
{
    RDM_SUCCESS =0, /*<<is returned when call is successful*/
    RDM_EPARAM_INVALID = -1, /*<< is returned when parameter values passed is not correct/out of bound etc.*/
    RDM_EOP_CANNOT_COMPLETE = -2 /*is returned interface cannot complete the requested operation<<*/

}radar_data_manager_err_codes_e;



/*
 * @typedef typedef void (*cb_radar_data_event)(void* data_ptr, uint32_t size)
 * Data subscriber callback prototype. The subscriber's callback function must follow this prototype.
 */
typedef void (*cb_radar_data_event)(void* data_ptr, uint32_t size);


/*
 * @typedef typedef struct  radar_data_manager_s
 * Radar Data Manager (RDM) interface .
 */
typedef struct {

/** @brief Expected interface:Read radar data function
 *
 * This function shall be implemented and supplied by the owner task/caller
 * that initializes the radar data manager. This function will be called by
 * radar data manager (RDM) during execution of run() method to get the data
 * from the radar device.
 *
 *
 * @param[in,out] data radar raw data to be directly copied from radar hw to supplied address
 * @param[in,out] num_samples number of samples that are copied.
 * @note: One sample is one byte in length.
 * @param[in] samples_ub maximum number of samples to be copied at a time from owner task/caller
 * @warning: The caller shall not copy more than the expected amount of samples set by <b>samples_ub</b> in a
 * given call
 *
 * @return function shall return zero (0) on successful completion of the readout of data.
 *         in case the parameters supplied are not valid it shall return -1 and in case if
 *         operation cannot be complete it shall return -2
 */
int32_t (*in_read_radar_data) (uint16_t* data, uint32_t *num_samples, uint32_t samples_ub);

#ifdef FREERTOS_AWARE

/** @brief Provided interface:Subscribe to radar data buffer
 *
 * The radar data consumer tasks can register themselves before going to sleep through this
 * interface. Once the sufficient radar data is available the consumer task will be notified
 * by RDM
 *
 * @param[in] subscriber_task FREERTOS task handle to the subscriber task
 *
 * @return returns <b>subscriber_id </b> on successful subscription.
 *            if subscriber requests for subscription again before un-subscribing,
 *            corresponding subscription_id assigned to it during first subscription request is returned.
 *         in case the parameters supplied are not valid it shall return -1 and in case if
 *         operation cannot be complete it shall return -2
 * @note: valid range of <b>subscriber_id </b> will be from 1 to MAXIMUM supported subscriptions
 *        \ref ACTIVE_SUBSCRIPTION_UB
 *
 */
int32_t (*subscribe)(TaskHandle_t subscriber_task);

/** @brief Provided interface:Read radar data from buffer
 *
 * This function provides an interface for subscriber task to read the buffered radar data.
 * The subscriber task, once notified/ woken up, shall utilize this function to read the
 * from radar data manager internal buffer.
 *
 * @param[in] subscription_id subscription id of the subscriber. This ID is provided by RDM on successful subscription
 * @param[out] data_ptr pointer to the internal buffer where the data has to be read from subscriber task
 * @param[out] size number of bytes that are available to read
 *
 * @return function shall return zero (0) on successful completion of the readout of data.
 *         in case the parameters supplied are not valid it shall return -1 and in case if
 *         operation cannot be complete it shall return -2
 */
int32_t (*read_from_buffer)(int32_t subscription_id, uint16_t **data_ptr, uint32_t *size);

/** @brief Provided interface:Acknowledge to RDM that the subscriber has read the data from buffer
 *
 * Subscriber task shall notify RDM by calling this function, that it has finished reading the data from buffer
 * Once all subscribers finish reading data, RDM would then manage the internal queue accordingly.
 * @note The old data in the buffer will persist until all subscribers acknowledge their respective data reads
 *          However on arrival of new data
 * @param[in] subscription_id subscribers' identifier
 *
 * @return Nothing
 */
void (*ack_data_read)(int32_t subscription_id);

/** @brief Provided interface:Schedule radar data manager to run
 *
 * Subscriber task shall schedule the RDM by calling this method. This is generally done on
 * data reception ISR.
 * This function reads the available radar data by calling <b>in_read_radar_data</b> supplied interface
 * Manages the radar data buffering, and wakes up /notifies subscribers tasks when required amount of data
 * is available in buffer.
 *
 * @param[in] run_from_isr to be set to true if this function is being called from ISR, false otherwise.
 *
 * @return Nothing
 */
void (*run)(bool run_from_isr);
#else
/** @brief Provided interface:Subscribe to radar data buffer
 *
 * The radar data consumers can register themselves in order to get data ready event.
 * The consumer/subscriber shall register by implementing a call back and registering it
 * through this interface.
 * Once the sufficient radar data is available the consumer will be notified ba means of call to its
 * registered call back by RDM.
 *
 * @param[in] call_back a call back to be registered having a prototype of cb_radar_data_event type
 *
 * @return returns <b>subscriber_id </b> on successful subscription.
 *         if subscriber requests for subscription again before un-subscribing,
 *            corresponding subscription_id assigned to it during first subscription request is returned.
 *         in case the parameters supplied are not valid it shall return -1 and in case if
 *         operation cannot be complete it shall return -2
 * @note: valid range of <b>subscriber_id </b> will be from 1 to MAXIMUM supported subscriptions
 *        \ref ACTIVE_SUBSCRIPTION_UB
 *
 */
int32_t (*subscribe)(cb_radar_data_event call_back);

/** @brief Provided interface:Run radar data manager
 *
 * Subscriber  shall trigger the RDM by calling this method. This is generally done on
 * data reception ISR.
 * This function reads the available radar data by calling <b>in_read_radar_data</b> supplied interface
 * Manages the radar data buffering, and wakes up /notifies subscribers tasks when required amount of data
 * is available in buffer.
 *
 *
 * @return void/nothing
 */
void (*run)(void);

#endif

/** @brief Provided interface:Un-subscribe to radar data buffer
 *
 * The radar data consumers can de-register themselves from radar data ready notifications.
 *
 * @param[in] subscription_id subscription id of subscriber task/consumer
 *
 * @note: valid range of <b>subscriber_id </b> will be from 1 to MAXIMUM supported subscriptions
 *           \ref ACTIVE_SUBSCRIPTION_UB
 *
 * @return Void/nothing
 *
 */
void (*unsubscribe)(int32_t subscription_id);


/** @brief Provided interface:set fill level for radar data buffer
 *
 * The radar data fill level can be set to a value between 1 to buffer size.
 *
 * @param[in] fill_level value for buffer fill level
 *
 * @return function shall return zero (0) on successful update of fill level value.
 *         in case the value supplied is not valid it shall return -1.
 *
 */
int32_t (*set_fill_level)(int32_t fill_level);

/** @brief Provided interface:get fill level for radar data buffer
 *
 * The configured fill_level value is returned
 *
 * @return function shall return the fill level value.
 *
 */
int32_t (*get_fill_level)(void);

}radar_data_manager_s;


/** @brief Expected interface: Set platform specific memory allocations
 *
 * The radar data consumers can supply platform specific implementations of malloc for (heap) memroy alocation
 * and free for freeing up of preallocated memory, in case
 * it is not provided the standard definitions of free together with malloc will be used.
 *
 * @param[in] malloc_func pointer to consumer defined platform specific malloc() function
 * @param[in] free_func   pointer to consumer defined platform specific free() function
 *
 * @note In case either of the functions i.e. malloc and/ or free are not supplied (NULL)
 *           standard definitions of both the functions will be used.
 *
 * @warning In order to be effective, this function has to be called before calling \ref radar_data_manager_init
 * @return Void/nothing
 *
 */

void radar_data_manager_set_malloc_free(void* (*malloc_func)(size_t size),
                                           void (* free_func)(void* ptr));


/** @brief Initialize radar data manager
 *
 * This function initializes RDM, and allows consumer to control fill level of the buffer
 * and size of the buffer in bytes. Also, it populates the provided interfaces through manager interface instance
 * and expects the provision of expected interfaces during the initialization.
 *
 * @param[in,out] manager manager interface type.
 * @param[in] buffer_size size of the buffer to be allocated by RDM in bytes
 * @param[in] fill_level amount of data to be filled in buffer before RDM issues notifications
 *   to its consumer
 *
 * @return function shall return zero (0) on successful completion of the readout of data.
 *         in case the parameters supplied are not valid it shall return -1 and in case if
 *         operation cannot be complete it shall return -2
 *
 */
int32_t radar_data_manager_init(radar_data_manager_s* const manager, uint32_t buffer_size, uint32_t fill_level);


/** @brief De-initialize radar data manager
 *
 * This function de-initializes RDM. This causes the RDM to free the internal buffer
 * and resetting the internal state of the RDM
 *
 * @return function shall return zero (0) on successful completion of the readout of data.
 *         in case the parameters supplied are not valid it shall return -1 and in case if
 *         operation cannot be complete it shall return -2
 *
 */
int32_t radar_data_manager_deinit();

#endif
