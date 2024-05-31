#ifndef INC_PROTO_H_
#define INC_PROTO_H_

#include <stdint.h>
#include <stddef.h>
#include "ads.h"

//set to max payload size that will be sent
#define PROTO_MAX_PAYLOAD_SIZE sizeof(struct AdsData)

typedef char ProtoId[5];
typedef void (*ProtoCallback)(void *buffer, size_t size);

/**
 * @brief Register module in protocol module
 * @param id Module ID, 4-byte string with NULL terminator
 * @param cb Callback function for received packet (or NULL)
 * @return Handle number on success, negative value on failure
 */
int ProtoRegister(ProtoId id, ProtoCallback cb);

/**
 * @brief Send data
 * @param handle Handle number obtained from \a ProtoRegister()
 * @param *data Data buffer pointer
 * @param size Data size
 * @return \a size on success, negative value on failure
 */
int ProtoSend(int handle, void *data, uint32_t size);

/**
 * @brief Store received data for processing
 * @param *data Data buffer
 * @param size Data size
 */
void ProtoReceive(void *data, uint32_t size);

/**
 * @brief Process protocol events
 * @attention Run this function continuously in the main loop
 */
void ProtoProcess(void);

#endif /* INC_PROTO_H_ */
