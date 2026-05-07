/*
 * Copyright 2023-2024, 2026 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __RTOS_APPS_SHELL_PSFP_H__
#define __RTOS_APPS_SHELL_PSFP_H__

#define CMD_SF_UPDATE_HELP \
	"\nsf_update <index> [-h <stream_handle>] [-P <priority_spec>] [-m <flow_meter_ref>] [-M <flow_meter_enable>] [-g <stream_gate_ref>] [-S <max_sdu_size>] [-p]\n" \
	"        parameters:\n" \
	"                <index>: 32bits, user defined stream filter identifier\n" \
	"        options:\n" \
	"                -h <stream_handle>: stream handle\n" \
	"                -P <priority>: vlan tag priority code point  value (wildcarded if not specified)\n" \
	"                -m <flow_meter_ref>: flow meter instance identifier\n" \
	"                -M <flow_meter_enable>: flow meter enable, 0: disable, 1: enable, default: 0\n" \
	"                -g <stream_gate_ref>: stream gate instance identifier\n" \
	"                -S <max_sdu_size>: maximum service data unit in bytes\n" \
	"                -p: update entry in permanent database\n"

#define CMD_SF_DELETE_HELP \
	"\nsf_delete <index> [-p]\n" \
	"        parameters:\n" \
	"                <index>: user defined stream filter identifier\n" \
	"        options:\n" \
	"                -p: delete entry from permanent database\n"

#define CMD_SF_READ_HELP \
	"\nsf_read <index> [-p]\n" \
	"        parameters:\n" \
	"                <index>: user defined stream filter identifier\n" \
	"        options:\n" \
	"                -p: read entry from permanent database\n"

#define CMD_SG_UPDATE_HELP \
	"\nsg_update <index> [-e <enable>] [-s <admin_state>] [-P <admin_ipv>] [-c <cycle_time>] [-C <cycle_time_ext>] [-b <base_time>] [-l <state,ipv,interval,octet>] [-l ...] [-I <enable>] [-X <enable>] [-p]\n" \
	"        parameters:\n" \
	"                <index>: user defined stream gate identifier\n" \
	"        options:\n" \
	"                -e <enable>: gate enable, 0: disabled (default), 1: enabled\n" \
	"                -s <admin_state>: administrative gate state, 0: closed, 1: open (default)\n" \
	"                -P <admin_ipv>: administrative IPV, 0 to 7, 0xff:null (default)\n" \
	"                -c <cycle_time>: gate cycle time in ns, default: 100000\n" \
	"                -C <cycle_time_ext>: gate cycle time extension in ns, 0 to 4294967295, default: 0\n" \
	"                -b <base_time>: gate base time in ns, 0 to (2^64 - 1), default: 0\n" \
	"                -l <state,ipv,interval,octet>: gate control list, default: 0,0,100000,0 (one option per list entry)\n" \
	"                -I <enable>: gate closed due to invalid rx enable 0: disabled (default), 1: enabled\n" \
	"                -X <enable>: gate closed due to octets exceeded enable, 0: disabled (default), 1: enabled\n" \
	"                -p: update entry in permanent database\n" \
	"\nsg_update <index> [-i] [-x]\n" \
	"        parameters:\n" \
	"                <index>: user defined stream gate identifier\n" \
	"        options:\n" \
	"                -i: reset gate closed due to invalid rx\n" \
	"                -x: reset gate closed due to octets exceeded\n"

#define CMD_SG_DELETE_HELP \
	"\nsg_delete <index> [-p]\n" \
	"        parameters:\n" \
	"                <index>: user defined stream gate identifier\n" \
	"        options:\n" \
	"                -p: delete entry from permanent database\n"

#define CMD_SG_READ_HELP \
	"\nsg_read <index> [-p] [-t <type>]\n" \
	"        parameters:\n" \
	"                <index>: user defined stream gate identifier\n" \
	"        options:\n" \
	"                -p: read entry from permanent database\n" \
	"                -t <type>: configuration type, 0: operational, 1: administrative, default: 0\n"

#define CMD_FM_UPDATE_HELP \
	"\nfm_update <index> -r <cir> -b <cbs> -R <eir> -B <ebs> -f <cflag> -c <cmode> -y <dropy> -m <markre> [-p]\n" \
	"        parameters:\n" \
	"                <index>: user defined flow meter identifier\n" \
	"        options:\n" \
	"                -r <cir>: committed rate in unit of bits per sec, default: 10000000\n" \
	"                -b <cbs>: committed burst size in bytes, default: 2000\n" \
	"                -R <eir>: excess rate in unit of bits per sec, default: 0\n" \
	"                -B <ebs>: excess burst size in bytes, default: 0\n" \
	"                -f <cflag>: coupling flag, 0: CIR and EIR buckets not coupled, 1: CIR and EIR buckets coupled, default:0\n" \
	"                -c <cmode>: color mode, 0: color-blind, 1: color-aware, default: 0\n" \
	"                -y <dropy>: drop on yellow, 0: yellow frames eligible to drop, 1: yellow frames are dropped, default: 0\n" \
	"                -m <markre>: mark all frames red enable, default: 0\n" \
	"                -p: update entry in permanent database\n" \
	"\nfm_update <index> -M \n" \
	"        parameters:\n" \
	"                <index>: user defined flow meter identifier\n" \
	"        options:\n" \
	"                -M: reset mark all frames red\n"

#define CMD_FM_DELETE_HELP \
	"\nfm_delete <index> [-p]\n" \
	"        parameters:\n" \
	"                <index>: user defined flow meter identifier\n" \
	"        options:\n" \
	"                -p: delete entry from permanent database\n"

#define CMD_FM_READ_HELP \
	"\nfm_read <index> [-p]\n" \
	"        parameters:\n" \
	"                <index>: user defined flow meter identifier\n" \
	"        options:\n" \
	"                -p: read entry from permanent database\n"

void cmd_psfp_init(shell_handle_t shell);

shell_status_t cmd_sf_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sf_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sf_read(shell_handle_t shell, int32_t argc, char **argv);

shell_status_t cmd_sg_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sg_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_sg_read(shell_handle_t shell, int32_t argc, char **argv);

shell_status_t cmd_fm_update(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fm_delete(shell_handle_t shell, int32_t argc, char **argv);
shell_status_t cmd_fm_read(shell_handle_t shell, int32_t argc, char **argv);

#endif /* __RTOS_APPS_SHELL_PSFP_H__ */
