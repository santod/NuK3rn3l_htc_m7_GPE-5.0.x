/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _ARCH_ARM_MACH_MSM_IPC_ROUTER_H
#define _ARCH_ARM_MACH_MSM_IPC_ROUTER_H
#if defined(CONFIG_MACH_M7_WLV)

#include <linux/types.h>
#include <linux/socket.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/msm_ipc.h>

#include <net/sock.h>

#define IPC_ROUTER_V1		1
#define IPC_ROUTER_V2		3

#define IPC_ROUTER_ADDRESS			0x0000FFFF

#define IPC_ROUTER_NID_LOCAL			1
#define MAX_IPC_PKT_SIZE 66000

#define IPC_ROUTER_CTRL_CMD_DATA		1
#define IPC_ROUTER_CTRL_CMD_HELLO		2
#define IPC_ROUTER_CTRL_CMD_BYE			3
#define IPC_ROUTER_CTRL_CMD_NEW_SERVER		4
#define IPC_ROUTER_CTRL_CMD_REMOVE_SERVER	5
#define IPC_ROUTER_CTRL_CMD_REMOVE_CLIENT	6
#define IPC_ROUTER_CTRL_CMD_RESUME_TX		7
#define IPC_ROUTER_CTRL_CMD_EXIT		8
#define IPC_ROUTER_CTRL_CMD_PING		9

#define IPC_ROUTER_DEFAULT_RX_QUOTA	5

#define IPC_ROUTER_XPRT_EVENT_DATA  1
#define IPC_ROUTER_XPRT_EVENT_OPEN  2
#define IPC_ROUTER_XPRT_EVENT_CLOSE 3

#define IPC_ROUTER_INFINITY -1
#define DEFAULT_RCV_TIMEO 0

#define ALIGN_SIZE(x) ((4 - ((x) & 3)) & 3)

#define ALL_SERVICE 0xFFFFFFFF
#define ALL_INSTANCE 0xFFFFFFFF

#define CONTROL_FLAG_CONFIRM_RX 0x1
#define CONTROL_FLAG_OPT_HDR 0x2

#define FRAG_PKT_WRITE_ENABLE 0x1

enum {
	CLIENT_PORT,
	SERVER_PORT,
	CONTROL_PORT,
	IRSC_PORT,
};

enum {
	NULL_MODE,
	SINGLE_LINK_MODE,
	MULTI_LINK_MODE,
};

union rr_control_msg {
	uint32_t cmd;
	struct {
		uint32_t cmd;
		uint32_t magic;
		uint32_t capability;
	} hello;
	struct {
		uint32_t cmd;
		uint32_t service;
		uint32_t instance;
		uint32_t node_id;
		uint32_t port_id;
	} srv;
	struct {
		uint32_t cmd;
		uint32_t node_id;
		uint32_t port_id;
	} cli;
};

struct rr_header_v1 {
	uint32_t version;
	uint32_t type;
	uint32_t src_node_id;
	uint32_t src_port_id;
	uint32_t control_flag;
	uint32_t size;
	uint32_t dst_node_id;
	uint32_t dst_port_id;
};

struct rr_header_v2 {
	uint8_t version;
	uint8_t type;
	uint16_t control_flag;
	uint32_t size;
	uint16_t src_node_id;
	uint16_t src_port_id;
	uint16_t dst_node_id;
	uint16_t dst_port_id;
} __attribute__((__packed__));

union rr_header {
	struct rr_header_v1 hdr_v1;
	struct rr_header_v2 hdr_v2;
};

#define IPC_ROUTER_HDR_SIZE sizeof(union rr_header)

struct rr_packet {
	struct list_head list;
	struct rr_header_v1 hdr;
	struct sk_buff_head *pkt_fragment_q;
	uint32_t length;
};

struct msm_ipc_sock {
	struct sock sk;
	struct msm_ipc_port *port;
	void *default_pil;
};

struct msm_ipc_router_xprt {
	char *name;
	uint32_t link_id;
	void *priv;

	int (*get_version)(struct msm_ipc_router_xprt *xprt);
	int (*get_option)(struct msm_ipc_router_xprt *xprt);
	int (*read_avail)(struct msm_ipc_router_xprt *xprt);
	int (*read)(void *data, uint32_t len,
		    struct msm_ipc_router_xprt *xprt);
	int (*write_avail)(struct msm_ipc_router_xprt *xprt);
	int (*write)(void *data, uint32_t len,
		     struct msm_ipc_router_xprt *xprt);
	int (*close)(struct msm_ipc_router_xprt *xprt);
	void (*sft_close_done)(struct msm_ipc_router_xprt *xprt);
};

void msm_ipc_router_xprt_notify(struct msm_ipc_router_xprt *xprt,
				unsigned event,
				void *data);


struct rr_packet *clone_pkt(struct rr_packet *pkt);
void release_pkt(struct rr_packet *pkt);


struct msm_ipc_port *msm_ipc_router_create_raw_port(void *endpoint,
		void (*notify)(unsigned event, void *priv),
		void *priv);
int msm_ipc_router_send_to(struct msm_ipc_port *src,
			   struct sk_buff_head *data,
			   struct msm_ipc_addr *dest);
int msm_ipc_router_read(struct msm_ipc_port *port_ptr,
			struct rr_packet **pkt,
			size_t buf_len);
int msm_ipc_router_bind_control_port(struct msm_ipc_port *port_ptr);

int msm_ipc_router_recv_from(struct msm_ipc_port *port_ptr,
		      struct rr_packet **pkt,
		      struct msm_ipc_addr *src_addr,
		      long timeout);
int msm_ipc_router_register_server(struct msm_ipc_port *server_port,
			    struct msm_ipc_addr *name);
int msm_ipc_router_unregister_server(struct msm_ipc_port *server_port);

int msm_ipc_router_init_sockets(void);
void msm_ipc_router_exit_sockets(void);

void msm_ipc_sync_sec_rule(uint32_t service, uint32_t instance, void *rule);

void msm_ipc_sync_default_sec_rule(void *rule);

int msm_ipc_router_rx_data_wait(struct msm_ipc_port *port_ptr, long timeout);

#if defined CONFIG_MSM_IPC_ROUTER_SMD_XPRT
extern void *msm_ipc_load_default_node(void);

extern void msm_ipc_unload_default_node(void *pil);
#else
static inline void *msm_ipc_load_default_node(void)
{ return NULL; }

static inline void msm_ipc_unload_default_node(void *pil) { }
#endif


#else

#include <linux/types.h>
#include <linux/socket.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/msm_ipc.h>

#include <net/sock.h>

#define IPC_ROUTER_VERSION			1
#define IPC_ROUTER_PROCESSORS_MAX		4

#define IPC_ROUTER_CLIENT_BCAST_ID		0xffffffff
#define IPC_ROUTER_ADDRESS			0xfffffffe

#define IPC_ROUTER_NID_LOCAL			1
#define IPC_ROUTER_NID_REMOTE			0

#define IPC_ROUTER_CTRL_CMD_DATA		1
#define IPC_ROUTER_CTRL_CMD_HELLO		2
#define IPC_ROUTER_CTRL_CMD_BYE			3
#define IPC_ROUTER_CTRL_CMD_NEW_SERVER		4
#define IPC_ROUTER_CTRL_CMD_REMOVE_SERVER	5
#define IPC_ROUTER_CTRL_CMD_REMOVE_CLIENT	6
#define IPC_ROUTER_CTRL_CMD_RESUME_TX		7
#define IPC_ROUTER_CTRL_CMD_EXIT		8
#define IPC_ROUTER_CTRL_CMD_PING		9

#define IPC_ROUTER_DEFAULT_RX_QUOTA	5

#define IPC_ROUTER_XPRT_EVENT_DATA  1
#define IPC_ROUTER_XPRT_EVENT_OPEN  2
#define IPC_ROUTER_XPRT_EVENT_CLOSE 3

#define NUM_NODES 2

#define IPC_ROUTER_INFINITY -1
#define DEFAULT_RCV_TIMEO IPC_ROUTER_INFINITY

#define ALIGN_SIZE(x) ((4 - ((x) & 3)) & 3)

enum {
	MSM_IPC_ROUTER_READ_CB = 0,
	MSM_IPC_ROUTER_WRITE_DONE,
};

union rr_control_msg {
	uint32_t cmd;
	struct {
		uint32_t cmd;
		uint32_t service;
		uint32_t instance;
		uint32_t node_id;
		uint32_t port_id;
	} srv;
	struct {
		uint32_t cmd;
		uint32_t node_id;
		uint32_t port_id;
	} cli;
};

struct rr_header {
	uint32_t version;
	uint32_t type;
	uint32_t src_node_id;
	uint32_t src_port_id;
	uint32_t confirm_rx;
	uint32_t size;
	uint32_t dst_node_id;
	uint32_t dst_port_id;
};

#define IPC_ROUTER_HDR_SIZE sizeof(struct rr_header)
#define MAX_IPC_PKT_SIZE 66000

#define IPC_ROUTER_MAX_REMOTE_SERVERS		100
#define MAX_WAKELOCK_NAME_SZ 32

struct rr_packet {
	struct list_head list;
	struct sk_buff_head *pkt_fragment_q;
	uint32_t length;
};

struct msm_ipc_port {
	struct list_head list;

	struct msm_ipc_port_addr this_port;
	struct msm_ipc_port_name port_name;
	uint32_t type;
	unsigned flags;
	spinlock_t port_lock;

	struct list_head incomplete;
	struct mutex incomplete_lock;

	struct list_head port_rx_q;
	struct mutex port_rx_q_lock;
	char rx_wakelock_name[MAX_WAKELOCK_NAME_SZ];
	struct wake_lock port_rx_wake_lock;
	wait_queue_head_t port_rx_wait_q;

	int restart_state;
	spinlock_t restart_lock;
	wait_queue_head_t restart_wait;

	void *endpoint;
	void (*notify)(unsigned event, void *data, void *addr, void *priv);

	uint32_t num_tx;
	uint32_t num_rx;
	unsigned long num_tx_bytes;
	unsigned long num_rx_bytes;
	void *priv;
};

struct msm_ipc_sock {
	struct sock sk;
	struct msm_ipc_port *port;
	void *default_pil;
};

enum write_data_type {
	HEADER = 1,
	PACKMARK,
	PAYLOAD,
};

struct msm_ipc_router_xprt {
	char *name;
	uint32_t link_id;
	void *priv;

	int (*get_version)(struct msm_ipc_router_xprt *xprt);
	int (*get_option)(struct msm_ipc_router_xprt *xprt);
	int (*read_avail)(struct msm_ipc_router_xprt *xprt);
	int (*read)(void *data, uint32_t len,
		    struct msm_ipc_router_xprt *xprt);
	int (*write_avail)(struct msm_ipc_router_xprt *xprt);
	int (*write)(void *data, uint32_t len,
		     struct msm_ipc_router_xprt *xprt);
	int (*close)(struct msm_ipc_router_xprt *xprt);
	void (*sft_close_done)(struct msm_ipc_router_xprt *xprt);
};

extern struct completion msm_ipc_remote_router_up;

void msm_ipc_router_xprt_notify(struct msm_ipc_router_xprt *xprt,
				unsigned event,
				void *data);


struct rr_packet *clone_pkt(struct rr_packet *pkt);
void release_pkt(struct rr_packet *pkt);


struct msm_ipc_port *msm_ipc_router_create_raw_port(void *endpoint,
		void (*notify)(unsigned event, void *data,
			       void *addr, void *priv),
		void *priv);
int msm_ipc_router_send_to(struct msm_ipc_port *src,
			   struct sk_buff_head *data,
			   struct msm_ipc_addr *dest);
int msm_ipc_router_read(struct msm_ipc_port *port_ptr,
			struct sk_buff_head **data,
			size_t buf_len);
int msm_ipc_router_get_curr_pkt_size(struct msm_ipc_port *port_ptr);
int msm_ipc_router_bind_control_port(struct msm_ipc_port *port_ptr);

#ifdef CONFIG_MSM8960_ONLY
int msm_ipc_router_lookup_server_name(struct msm_ipc_port_name *srv_name,
				      struct msm_ipc_port_addr *port_addr,
				      int num_entries_in_array,
				      uint32_t lookup_mask);
#else
int msm_ipc_router_lookup_server_name(struct msm_ipc_port_name *srv_name,
				      struct msm_ipc_server_info *srv_info,
				      int num_entries_in_array,
				      uint32_t lookup_mask);
#endif 
int msm_ipc_router_close_port(struct msm_ipc_port *port_ptr);

struct msm_ipc_port *msm_ipc_router_create_port(
	void (*notify)(unsigned event, void *data,
		       void *addr, void *priv),
	void *priv);
int msm_ipc_router_recv_from(struct msm_ipc_port *port_ptr,
		      struct sk_buff_head **data,
		      struct msm_ipc_addr *src_addr,
		      unsigned long timeout);
int msm_ipc_router_register_server(struct msm_ipc_port *server_port,
			    struct msm_ipc_addr *name);
int msm_ipc_router_unregister_server(struct msm_ipc_port *server_port);


int msm_ipc_router_init_sockets(void);
void msm_ipc_router_exit_sockets(void);

#if defined CONFIG_MSM_IPC_ROUTER_SMD_XPRT
extern void *msm_ipc_load_default_node(void);

extern void msm_ipc_unload_default_node(void *pil);
#else
static inline void *msm_ipc_load_default_node(void)
{ return NULL; }

static inline void msm_ipc_unload_default_node(void *pil) { }
#endif

#endif
#endif
