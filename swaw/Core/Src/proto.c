#include "proto.h"
#include "usbd_cdc_if.h"
#include "ads.h"
#include <string.h>

#define PROTO_ENTRY_COUNT 6

struct ProtoEntry
{
	ProtoId id;
	ProtoCallback cb;
} static ProtoTable[PROTO_ENTRY_COUNT];

static int usedEntries = 0;

struct ProtoPacket
{
	ProtoId id;
	uint32_t size;
	uint8_t payload[];
} __attribute__ ((packed));


static uint8_t protoBuffer[PROTO_MAX_PAYLOAD_SIZE + sizeof(struct ProtoPacket)];


struct ProtoRxState
{
	struct ProtoPacket *packet;
	uint32_t size;
	bool available;
} volatile static ProtoRxState = {.available = false};

void ProtoProcess(void)
{
	if(ProtoRxState.available)
	{
		if(ProtoRxState.size < sizeof(ProtoId))
			return;

		if(ProtoRxState.packet->size > (ProtoRxState.size - sizeof(struct ProtoPacket)))
			return;

		for(int i = 0; i < usedEntries; i++)
		{
			if(0 == memcmp(ProtoRxState.packet->id, ProtoTable[i].id, sizeof(ProtoRxState.packet->id)))
			{
				if(NULL != ProtoTable[i].cb)
					ProtoTable[i].cb(ProtoRxState.packet->payload, ProtoRxState.packet->size);
			}
		}
		ProtoRxState.available = false;
	}
}

void ProtoReceive(void *data, uint32_t size)
{
	ProtoRxState.packet = data;
	ProtoRxState.size = size;
	ProtoRxState.available = true;
}

int ProtoRegister(ProtoId id, ProtoCallback cb)
{
	if(usedEntries >= PROTO_ENTRY_COUNT)
		return -1;

	memcpy(ProtoTable[usedEntries].id, id, sizeof(ProtoTable->id) / sizeof(ProtoTable->id[0]));
	ProtoTable[usedEntries].cb = cb;
	return usedEntries++;
}

int ProtoSend(int handle, void *data, uint32_t size)
{
	if(handle < 0)
		return -1;
	if(handle >= usedEntries)
		return -1;

	struct ProtoPacket *p = (struct ProtoPacket*)protoBuffer;
	memcpy(p->id, ProtoTable[handle].id, sizeof(p->id) / sizeof(p->id[0]));
	p->size = size;
	memcpy(p->payload, data, size);
	if(USBD_OK == CDC_Transmit_FS((uint8_t*)p, size + sizeof(struct ProtoPacket)))
		return size;
	else
		return -1;
}
