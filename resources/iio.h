/*
 * libiio - Library for interfacing industrial I/O (IIO) devices
 *
 * Copyright (C) 2014 Analog Devices, Inc.
 * Author: Paul Cercueil <paul.cercueil@analog.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * */

/** @file iio.h
 * @brief Public interface */

#ifndef __IIO_H__
#define __IIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#if (defined(_WIN32) || defined(__MBED__))
#ifndef _SSIZE_T_DEFINED
typedef ptrdiff_t ssize_t;
#define _SSIZE_T_DEFINED
#endif
#else
#include <sys/types.h>
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1800) && !defined(__BOOL_DEFINED)
#undef bool
#undef false
#undef true
#define bool char
#define false 0
#define true 1
#else
#include <stdbool.h>
#endif

#if defined(__GNUC__) && !defined(MATLAB_MEX_FILE) && !defined(MATLAB_LOADLIBRARY)
#ifndef __cnst
#define __cnst __attribute__((const))
#endif
#ifndef __pure
#define __pure __attribute__((pure))
#endif
#define __notused __attribute__((unused))
#ifdef IIO_CHECK_RET
#define __check_ret __attribute__((warn_unused_result))
#else
#define __check_ret
#endif
#else
#define __cnst
#define __pure
#define __notused
#define __check_ret
#endif

#ifdef _WIN32
#   ifdef LIBIIO_EXPORTS
#	define __api __declspec(dllexport)
#   else
#	define __api __declspec(dllimport)
#   endif
#elif __GNUC__ >= 4 && !defined(MATLAB_MEX_FILE) && !defined(MATLAB_LOADLIBRARY)
#   define __api __attribute__((visibility ("default")))
#else
#   define __api
#endif

struct iio_context;
struct iio_device;
struct iio_channel;
struct iio_buffer;

struct iio_context_info;
struct iio_scan_context;
struct iio_scan_block;

/**
 * @enum iio_chan_type
 * @brief IIO channel type
 *
 * A IIO channel has a type specifying the type of data associated with the
 * channel.
 */
enum iio_chan_type {
	IIO_VOLTAGE,
	IIO_CURRENT,
	IIO_POWER,
	IIO_ACCEL,
	IIO_ANGL_VEL,
	IIO_MAGN,
	IIO_LIGHT,
	IIO_INTENSITY,
	IIO_PROXIMITY,
	IIO_TEMP,
	IIO_INCLI,
	IIO_ROT,
	IIO_ANGL,
	IIO_TIMESTAMP,
	IIO_CAPACITANCE,
	IIO_ALTVOLTAGE,
	IIO_CCT,
	IIO_PRESSURE,
	IIO_HUMIDITYRELATIVE,
	IIO_ACTIVITY,
	IIO_STEPS,
	IIO_ENERGY,
	IIO_DISTANCE,
	IIO_VELOCITY,
	IIO_CONCENTRATION,
	IIO_RESISTANCE,
	IIO_PH,
	IIO_UVINDEX,
	IIO_ELECTRICALCONDUCTIVITY,
	IIO_COUNT,
	IIO_INDEX,
	IIO_GRAVITY,
	IIO_POSITIONRELATIVE,
	IIO_PHASE,
	IIO_MASSCONCENTRATION,
	IIO_CHAN_TYPE_UNKNOWN = INT_MAX
};

/**
 * @enum iio_modifier
 * @brief IIO channel modifier
 *
 * In a addition to a type a IIO channel can optionally have a channel modifier
 * further specifying the data type of of the channel.
 */
enum iio_modifier {
	IIO_NO_MOD,
	IIO_MOD_X,
	IIO_MOD_Y,
	IIO_MOD_Z,
	IIO_MOD_X_AND_Y,
	IIO_MOD_X_AND_Z,
	IIO_MOD_Y_AND_Z,
	IIO_MOD_X_AND_Y_AND_Z,
	IIO_MOD_X_OR_Y,
	IIO_MOD_X_OR_Z,
	IIO_MOD_Y_OR_Z,
	IIO_MOD_X_OR_Y_OR_Z,
	IIO_MOD_LIGHT_BOTH,
	IIO_MOD_LIGHT_IR,
	IIO_MOD_ROOT_SUM_SQUARED_X_Y,
	IIO_MOD_SUM_SQUARED_X_Y_Z,
	IIO_MOD_LIGHT_CLEAR,
	IIO_MOD_LIGHT_RED,
	IIO_MOD_LIGHT_GREEN,
	IIO_MOD_LIGHT_BLUE,
	IIO_MOD_QUATERNION,
	IIO_MOD_TEMP_AMBIENT,
	IIO_MOD_TEMP_OBJECT,
	IIO_MOD_NORTH_MAGN,
	IIO_MOD_NORTH_TRUE,
	IIO_MOD_NORTH_MAGN_TILT_COMP,
	IIO_MOD_NORTH_TRUE_TILT_COMP,
	IIO_MOD_RUNNING,
	IIO_MOD_JOGGING,
	IIO_MOD_WALKING,
	IIO_MOD_STILL,
	IIO_MOD_ROOT_SUM_SQUARED_X_Y_Z,
	IIO_MOD_I,
	IIO_MOD_Q,
	IIO_MOD_CO2,
	IIO_MOD_VOC,
	IIO_MOD_LIGHT_UV,
	IIO_MOD_LIGHT_DUV,
	IIO_MOD_PM1,
	IIO_MOD_PM2P5,
	IIO_MOD_PM4,
	IIO_MOD_PM10,
	IIO_MOD_ETHANOL,
	IIO_MOD_H2,
	IIO_MOD_O2,
};

/* ---------------------------------------------------------------------------*/
/* ------------------------- Scan functions ----------------------------------*/
/** @defgroup Scan Functions for scanning available contexts
 * @{
 * @struct iio_scan_context
 * @brief The scanning context
 *
 * @struct iio_context_info
 * @brief The information related to a discovered context
 */


/** @brief Create a scan context
 * @param backend A NULL-terminated string containing the backend(s) to use for
 * scanning (example: pre version 0.20 :  "local", "ip", or "usb"; post version
 * 0.20 can handle multiple, including "local:usb:", "ip:usb:", "local:usb:ip:").
 * If NULL, all the available backends are used.
 * @param flags Unused for now. Set to 0.
 * @return on success, a pointer to a iio_scan_context structure
 * @return On failure, NULL is returned and errno is set appropriately */
__api __check_ret struct iio_scan_context * iio_create_scan_context(
		const char *backend, unsigned int flags);


/** @brief Destroy the given scan context
 * @param ctx A pointer to an iio_scan_context structure
 *
 * <b>NOTE:</b> After that function, the iio_scan_context pointer shall be invalid. */
__api void iio_scan_context_destroy(struct iio_scan_context *ctx);


/** @brief Enumerate available contexts
 * @param ctx A pointer to an iio_scan_context structure
 * @param info A pointer to a 'const struct iio_context_info **' typed variable.
 * The pointed variable will be initialized on success.
 * @returns On success, the number of contexts found.
 * @returns On failure, a negative error number.
 */
__api __check_ret ssize_t iio_scan_context_get_info_list(struct iio_scan_context *ctx,
		struct iio_context_info ***info);


/** @brief Free a context info list
 * @param info A pointer to a 'const struct iio_context_info *' typed variable
 */
__api void iio_context_info_list_free(struct iio_context_info **info);


/** @brief Get a description of a discovered context
 * @param info A pointer to an iio_context_info structure
 * @return A pointer to a static NULL-terminated string
 */
__api __check_ret __pure const char * iio_context_info_get_description(
		const struct iio_context_info *info);


/** @brief Get the URI of a discovered context
 * @param info A pointer to an iio_context_info structure
 * @return A pointer to a static NULL-terminated string
 */
__api __check_ret __pure const char * iio_context_info_get_uri(
		const struct iio_context_info *info);


/** @brief Create a scan block
 * @param backend A NULL-terminated string containing the backend to use for
 * scanning. If NULL, all the available backends are used.
 * @param flags Unused for now. Set to 0.
 * @return on success, a pointer to a iio_scan_block structure
 * @return On failure, NULL is returned and errno is set appropriately
 *
 * Introduced in version 0.20. */
__api struct iio_scan_block * iio_create_scan_block(
		const char *backend, unsigned int flags);


/** @brief Destroy the given scan block
 * @param blk A pointer to an iio_scan_block structure
 *
 * <b>NOTE:</b> After that function, the iio_scan_block pointer shall be invalid.
 *
 * Introduced in version 0.20. */
__api void iio_scan_block_destroy(struct iio_scan_block *blk);


/** @brief Enumerate available contexts via scan block
 * @param blk A pointer to a iio_scan_block structure.
 * @returns On success, the number of contexts found.
 * @returns On failure, a negative error number.
 *
 * Introduced in version 0.20. */
__api ssize_t iio_scan_block_scan(struct iio_scan_block *blk);


/** @brief Get the iio_context_info for a particular context
 * @param blk A pointer to an iio_scan_block structure
 * @param index The index corresponding to the context.
 * @return A pointer to the iio_context_info for the context
 * @returns On success, a pointer to the specified iio_context_info
 * @returns On failure, NULL is returned and errno is set appropriately
 *
 * Introduced in version 0.20. */
__api struct iio_context_info *iio_scan_block_get_info(
		struct iio_scan_block *blk, unsigned int index);


/** @} *//* ------------------------------------------------------------------*/
/* ------------------------- Top-level functions -----------------------------*/
/** @defgroup TopLevel Top-level functions
 * @{ */


/** @brief Get the version of the libiio library
 * @param major A pointer to an unsigned integer (NULL accepted)
 * @param minor A pointer to an unsigned integer (NULL accepted)
 * @param git_tag A pointer to a 8-characters buffer (NULL accepted) */
__api void iio_library_get_version(unsigned int *major,
		unsigned int *minor, char git_tag[8]);


/** @brief Get a string description of an error code
 * @param err The error code
 * @param dst A pointer to the memory area where the NULL-terminated string
 * corresponding to the error message will be stored
 * @param len The available length of the memory area, in bytes */
__api void iio_strerror(int err, char *dst, size_t len);


/** @brief Check if the specified backend is available
 * @param backend The name of the backend to query
 * @return True if the backend is available, false otherwise
 *
 * Introduced in version 0.9. */
__api __check_ret __cnst bool iio_has_backend(const char *backend);


/** @brief Get the number of available backends
 * @return The number of available backends
 *
 * Introduced in version 0.9. */
__api __check_ret __cnst unsigned int iio_get_backends_count(void);


/** @brief Retrieve the name of a given backend
 * @param index The index corresponding to the attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the index is invalid, NULL is returned
 *
 * Introduced in version 0.9. */
__api __check_ret __cnst const char * iio_get_backend(unsigned int index);


/** @} *//* ------------------------------------------------------------------*/
/* ------------------------- Context functions -------------------------------*/
/** @defgroup Context Context
 * @{
 * @struct iio_context
 * @brief Contains the representation of an IIO context */


/** @brief Create a context from local or remote IIO devices
 * @return On success, A pointer to an iio_context structure
 * @return On failure, NULL is returned and errno is set appropriately
 *
 * <b>NOTE:</b> This function will create a network context if the IIOD_REMOTE
 * environment variable is set to the hostname where the IIOD server runs. If
 * set to an empty string, the server will be discovered using ZeroConf.
 * If the environment variable is not set, a local context will be created
 * instead. */
__api __check_ret struct iio_context * iio_create_default_context(void);


/** @brief Create a context from local IIO devices (Linux only)
 * @return On success, A pointer to an iio_context structure
 * @return On failure, NULL is returned and errno is set appropriately */
__api __check_ret struct iio_context * iio_create_local_context(void);


/** @brief Create a context from a XML file
 * @param xml_file Path to the XML file to open
 * @return On success, A pointer to an iio_context structure
 * @return On failure, NULL is returned and errno is set appropriately
 *
 * <b>NOTE:</b> The format of the XML must comply to the one returned by
 * iio_context_get_xml. */
__api __check_ret struct iio_context * iio_create_xml_context(const char *xml_file);


/** @brief Create a context from XML data in memory
 * @param xml Pointer to the XML data in memory
 * @param len Length of the XML string in memory (excluding the final \0)
 * @return On success, A pointer to an iio_context structure
 * @return On failure, NULL is returned and errno is set appropriately
 *
 * <b>NOTE:</b> The format of the XML must comply to the one returned by
 * iio_context_get_xml */
__api __check_ret struct iio_context * iio_create_xml_context_mem(
		const char *xml, size_t len);


/** @brief Create a context from the network
 * @param host Hostname, IPv4 or IPv6 address where the IIO Daemon is running
 * @return On success, a pointer to an iio_context structure
 * @return On failure, NULL is returned and errno is set appropriately */
__api __check_ret struct iio_context * iio_create_network_context(const char *host);


/** @brief Create a context from a URI description
 * @param uri A URI describing the context location
 * @return On success, a pointer to a iio_context structure
 * @return On failure, NULL is returned and errno is set appropriately
 *
 * <b>NOTE:</b> The following URIs are supported based on compile time backend
 * support:
 * - Local backend, "local:"\n
 *   Does not have an address part. For example <i>"local:"</i>
 * - XML backend, "xml:"\n Requires a path to the XML file for the address part.
 *   For example <i>"xml:/home/user/file.xml"</i>
 * - Network backend, "ip:"\n Requires a hostname, IPv4, or IPv6 to connect to
 *   a specific running IIO Daemon or no address part for automatic discovery
 *   when library is compiled with ZeroConf support. For example
 *   <i>"ip:192.168.2.1"</i>, <b>or</b> <i>"ip:localhost"</i>, <b>or</b> <i>"ip:"</i>
 *   <b>or</b> <i>"ip:plutosdr.local"</i>
 * - USB backend, "usb:"\n When more than one usb device is attached, requires
 *   bus, address, and interface parts separated with a dot. For example
 *   <i>"usb:3.32.5"</i>. Where there is only one USB device attached, the shorthand
 *   <i>"usb:"</i> can be used.
 * - Serial backend, "serial:"\n Requires:
 *     - a port (/dev/ttyUSB0),
 *     - baud_rate (default <b>115200</b>)
 *     - serial port configuration
 *        - data bits (5 6 7 <b>8</b> 9)
 *        - parity ('<b>n</b>' none, 'o' odd, 'e' even, 'm' mark, 's' space)
 *        - stop bits (<b>1</b> 2)
 *        - flow control ('<b>\0</b>' none, 'x' Xon Xoff, 'r' RTSCTS, 'd' DTRDSR)
 *
 *  For example <i>"serial:/dev/ttyUSB0,115200"</i> <b>or</b> <i>"serial:/dev/ttyUSB0,115200,8n1"</i>*/
__api __check_ret struct iio_context * iio_create_context_from_uri(const char *uri);


/** @brief Duplicate a pre-existing IIO context
 * @param ctx A pointer to an iio_context structure
 * @return On success, A pointer to an iio_context structure
 * @return On failure, NULL is returned and errno is set appropriately
 *
 * <b>NOTE:</b> This function is not supported on 'usb:' contexts, since libusb
 * can only claim the interface once. "Function not implemented" is the expected errno.
 * Any context which is cloned, must be destroyed via calling iio_context_destroy() */
__api __check_ret struct iio_context * iio_context_clone(const struct iio_context *ctx);


/** @brief Destroy the given context
 * @param ctx A pointer to an iio_context structure
 *
 * <b>NOTE:</b> After that function, the iio_context pointer shall be invalid. */
__api void iio_context_destroy(struct iio_context *ctx);


/** @brief Get the version of the backend in use
 * @param ctx A pointer to an iio_context structure
 * @param major A pointer to an unsigned integer (NULL accepted)
 * @param minor A pointer to an unsigned integer (NULL accepted)
 * @param git_tag A pointer to a 8-characters buffer (NULL accepted)
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_context_get_version(const struct iio_context *ctx,
		unsigned int *major, unsigned int *minor, char git_tag[8]);


/** @brief Obtain a XML representation of the given context
 * @param ctx A pointer to an iio_context structure
 * @return A pointer to a static NULL-terminated string */
__api __check_ret __pure const char * iio_context_get_xml(const struct iio_context *ctx);


/** @brief Get the name of the given context
 * @param ctx A pointer to an iio_context structure
 * @return A pointer to a static NULL-terminated string
 *
 * <b>NOTE:</b>The returned string will be <b><i>local</i></b>,
 * <b><i>xml</i></b> or <b><i>network</i></b> when the context has been
 * created with the local, xml and network backends respectively.*/
__api __check_ret __pure const char * iio_context_get_name(const struct iio_context *ctx);


/** @brief Get a description of the given context
 * @param ctx A pointer to an iio_context structure
 * @return A pointer to a static NULL-terminated string
 *
 * <b>NOTE:</b>The returned string will contain human-readable information about
 * the current context. */
__api __check_ret __pure const char * iio_context_get_description(
		const struct iio_context *ctx);


/** @brief Get the number of context-specific attributes
 * @param ctx A pointer to an iio_context structure
 * @return The number of context-specific attributes
 *
 * Introduced in version 0.9. */
__api __check_ret __pure unsigned int iio_context_get_attrs_count(
		const struct iio_context *ctx);


/** @brief Retrieve the name and value of a context-specific attribute
 * @param ctx A pointer to an iio_context structure
 * @param index The index corresponding to the attribute
 * @param name A pointer to a const char * pointer (NULL accepted)
 * @param value A pointer to a const char * pointer (NULL accepted)
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * Introduced in version 0.9. */
__api __check_ret int iio_context_get_attr(
		const struct iio_context *ctx, unsigned int index,
		const char **name, const char **value);


/** @brief Retrieve the value of a context-specific attribute
 * @param ctx A pointer to an iio_context structure
 * @param name The name of the context attribute to read
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the name does not correspond to any attribute, NULL is
 * returned
 *
 * Introduced in version 0.9. */
__api __check_ret const char * iio_context_get_attr_value(
		const struct iio_context *ctx, const char *name);


/** @brief Enumerate the devices found in the given context
 * @param ctx A pointer to an iio_context structure
 * @return The number of devices found */
__api __check_ret __pure unsigned int iio_context_get_devices_count(
		const struct iio_context *ctx);


/** @brief Get the device present at the given index
 * @param ctx A pointer to an iio_context structure
 * @param index The index corresponding to the device
 * @return On success, a pointer to an iio_device structure
 * @return If the index is invalid, NULL is returned */
__api __check_ret __pure struct iio_device * iio_context_get_device(
		const struct iio_context *ctx, unsigned int index);


/** @brief Try to find a device structure by its name of ID
 * @param ctx A pointer to an iio_context structure
 * @param name A NULL-terminated string corresponding to the name or the ID of
 * the device to search for
 * @return On success, a pointer to an iio_device structure
 * @return If the name or ID does not correspond to any known device, NULL is
 * returned */
__api __check_ret __pure struct iio_device * iio_context_find_device(
		const struct iio_context *ctx, const char *name);


/** @brief Set a timeout for I/O operations
 * @param ctx A pointer to an iio_context structure
 * @param timeout_ms A positive integer representing the time in milliseconds
 * after which a timeout occurs. A value of 0 is used to specify that no
 * timeout should occur.
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_context_set_timeout(
		struct iio_context *ctx, unsigned int timeout_ms);


/** @} *//* ------------------------------------------------------------------*/
/* ------------------------- Device functions --------------------------------*/
/** @defgroup Device Device
 * @{
 * @struct iio_device
 * @brief Represents a device in the IIO context */


/** @brief Retrieve a pointer to the iio_context structure
 * @param dev A pointer to an iio_device structure
 * @return A pointer to an iio_context structure */
__api __check_ret __pure const struct iio_context * iio_device_get_context(
		const struct iio_device *dev);


/** @brief Retrieve the device ID (e.g. <b><i>iio:device0</i></b>)
 * @param dev A pointer to an iio_device structure
 * @return A pointer to a static NULL-terminated string */
__api __check_ret __pure const char * iio_device_get_id(const struct iio_device *dev);


/** @brief Retrieve the device name (e.g. <b><i>xadc</i></b>)
 * @param dev A pointer to an iio_device structure
 * @return A pointer to a static NULL-terminated string
 *
 * <b>NOTE:</b> if the device has no name, NULL is returned. */
__api __check_ret __pure const char * iio_device_get_name(const struct iio_device *dev);


/** @brief Enumerate the channels of the given device
 * @param dev A pointer to an iio_device structure
 * @return The number of channels found */
__api __check_ret __pure unsigned int iio_device_get_channels_count(
		const struct iio_device *dev);


/** @brief Enumerate the device-specific attributes of the given device
 * @param dev A pointer to an iio_device structure
 * @return The number of device-specific attributes found */
__api __check_ret __pure unsigned int iio_device_get_attrs_count(
		const struct iio_device *dev);

/** @brief Enumerate the buffer-specific attributes of the given device
 * @param dev A pointer to an iio_device structure
 * @return The number of buffer-specific attributes found */
__api __check_ret __pure unsigned int iio_device_get_buffer_attrs_count(
		const struct iio_device *dev);

/** @brief Get the channel present at the given index
 * @param dev A pointer to an iio_device structure
 * @param index The index corresponding to the channel
 * @return On success, a pointer to an iio_channel structure
 * @return If the index is invalid, NULL is returned */
__api __check_ret __pure struct iio_channel * iio_device_get_channel(
		const struct iio_device *dev, unsigned int index);


/** @brief Get the device-specific attribute present at the given index
 * @param dev A pointer to an iio_device structure
 * @param index The index corresponding to the attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the index is invalid, NULL is returned */
__api __check_ret __pure const char * iio_device_get_attr(
		const struct iio_device *dev, unsigned int index);

/** @brief Get the buffer-specific attribute present at the given index
 * @param dev A pointer to an iio_device structure
 * @param index The index corresponding to the attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the index is invalid, NULL is returned */
__api __check_ret __pure const char * iio_device_get_buffer_attr(
		const struct iio_device *dev, unsigned int index);

/** @brief Try to find a channel structure by its name of ID
 * @param dev A pointer to an iio_device structure
 * @param name A NULL-terminated string corresponding to the name or the ID of
 * the channel to search for
 * @param output True if the searched channel is output, False otherwise
 * @return On success, a pointer to an iio_channel structure
 * @return If the name or ID does not correspond to any known channel of the
 * given device, NULL is returned */
__api __check_ret __pure struct iio_channel * iio_device_find_channel(
		const struct iio_device *dev, const char *name, bool output);


/** @brief Try to find a device-specific attribute by its name
 * @param dev A pointer to an iio_device structure
 * @param name A NULL-terminated string corresponding to the name of the
 * attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the name does not correspond to any known attribute of the given
 * device, NULL is returned
 *
 * <b>NOTE:</b> This function is useful to detect the presence of an attribute.
 * It can also be used to retrieve the name of an attribute as a pointer to a
 * static string from a dynamically allocated string. */
__api __check_ret __pure const char * iio_device_find_attr(
		const struct iio_device *dev, const char *name);

/** @brief Try to find a buffer-specific attribute by its name
 * @param dev A pointer to an iio_device structure
 * @param name A NULL-terminated string corresponding to the name of the
 * attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the name does not correspond to any known attribute of the given
 * device, NULL is returned
 *
 * <b>NOTE:</b> This function is useful to detect the presence of an attribute.
 * It can also be used to retrieve the name of an attribute as a pointer to a
 * static string from a dynamically allocated string. */
__api __check_ret __pure const char * iio_device_find_buffer_attr(
		const struct iio_device *dev, const char *name);

/** @brief Read the content of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param dst A pointer to the memory area where the NULL-terminated string
 * corresponding to the value read will be stored
 * @param len The available length of the memory area, in bytes
 * @return On success, the number of bytes written to the buffer
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to iio_device_attr_read,
 * it is now possible to read all of the attributes of a device.
 *
 * The buffer is filled with one block of data per attribute of the device,
 * by the order they appear in the iio_device structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, it corresponds to the errno code that were
 * returned when reading the attribute; if positive, it corresponds to the
 * length of the data read. In that case, the rest of the block contains
 * the data. */
__api __check_ret ssize_t iio_device_attr_read(const struct iio_device *dev,
		const char *attr, char *dst, size_t len);


/** @brief Read the content of all device-specific attributes
 * @param dev A pointer to an iio_device structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the device-specific attributes are read in one single
 * command. */
__api __check_ret int iio_device_attr_read_all(struct iio_device *dev,
		int (*cb)(struct iio_device *dev, const char *attr,
			const char *value, size_t len, void *d),
		void *data);


/** @brief Read the content of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a bool variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_attr_read_bool(const struct iio_device *dev,
		const char *attr, bool *val);


/** @brief Read the content of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a long long variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_attr_read_longlong(const struct iio_device *dev,
		const char *attr, long long *val);


/** @brief Read the content of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a double variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_attr_read_double(const struct iio_device *dev,
		const char *attr, double *val);


/** @brief Set the value of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param src A NULL-terminated string to set the attribute to
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to iio_device_attr_write,
 * it is now possible to write all of the attributes of a device.
 *
 * The buffer must contain one block of data per attribute of the device,
 * by the order they appear in the iio_device structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, the attribute is not written; if positive,
 * it corresponds to the length of the data to write. In that case, the rest
 * of the block must contain the data. */
__api __check_ret ssize_t iio_device_attr_write(const struct iio_device *dev,
		const char *attr, const char *src);


/** @brief Set the value of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param src A pointer to the data to be written
 * @param len The number of bytes that should be written
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned */
__api __check_ret ssize_t iio_device_attr_write_raw(const struct iio_device *dev,
		const char *attr, const void *src, size_t len);


/** @brief Set the values of all device-specific attributes
 * @param dev A pointer to an iio_device structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the device-specific attributes are written in one single
 * command. */
__api __check_ret int iio_device_attr_write_all(struct iio_device *dev,
		ssize_t (*cb)(struct iio_device *dev,
			const char *attr, void *buf, size_t len, void *d),
		void *data);


/** @brief Set the value of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A bool value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_attr_write_bool(const struct iio_device *dev,
		const char *attr, bool val);


/** @brief Set the value of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A long long value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_attr_write_longlong(const struct iio_device *dev,
		const char *attr, long long val);


/** @brief Set the value of the given device-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A double value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_attr_write_double(const struct iio_device *dev,
		const char *attr, double val);

/** @brief Read the content of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param dst A pointer to the memory area where the NULL-terminated string
 * corresponding to the value read will be stored
 * @param len The available length of the memory area, in bytes
 * @return On success, the number of bytes written to the buffer
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to
 * iio_device_buffer_attr_read, it is now possible to read all of the attributes
 * of a device.
 *
 * The buffer is filled with one block of data per attribute of the buffer,
 * by the order they appear in the iio_device structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, it corresponds to the errno code that were
 * returned when reading the attribute; if positive, it corresponds to the
 * length of the data read. In that case, the rest of the block contains
 * the data. */
__api __check_ret ssize_t iio_device_buffer_attr_read(const struct iio_device *dev,
		const char *attr, char *dst, size_t len);

/** @brief Read the content of all buffer-specific attributes
 * @param dev A pointer to an iio_device structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the buffer-specific attributes are read in one single
 * command. */
__api __check_ret int iio_device_buffer_attr_read_all(struct iio_device *dev,
		int (*cb)(struct iio_device *dev, const char *attr,
			const char *value, size_t len, void *d),
		void *data);


/** @brief Read the content of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a bool variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_buffer_attr_read_bool(const struct iio_device *dev,
		const char *attr, bool *val);


/** @brief Read the content of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a long long variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_buffer_attr_read_longlong(const struct iio_device *dev,
		const char *attr, long long *val);


/** @brief Read the content of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a double variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_buffer_attr_read_double(const struct iio_device *dev,
		const char *attr, double *val);


/** @brief Set the value of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param src A NULL-terminated string to set the attribute to
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to
 * iio_device_buffer_attr_write, it is now possible to write all of the
 * attributes of a device.
 *
 * The buffer must contain one block of data per attribute of the buffer,
 * by the order they appear in the iio_device structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, the attribute is not written; if positive,
 * it corresponds to the length of the data to write. In that case, the rest
 * of the block must contain the data. */
__api __check_ret ssize_t iio_device_buffer_attr_write(const struct iio_device *dev,
		const char *attr, const char *src);


/** @brief Set the value of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param src A pointer to the data to be written
 * @param len The number of bytes that should be written
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned */
__api __check_ret ssize_t iio_device_buffer_attr_write_raw(const struct iio_device *dev,
		const char *attr, const void *src, size_t len);


/** @brief Set the values of all buffer-specific attributes
 * @param dev A pointer to an iio_device structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the buffer-specific attributes are written in one single
 * command. */
__api __check_ret int iio_device_buffer_attr_write_all(struct iio_device *dev,
		ssize_t (*cb)(struct iio_device *dev,
			const char *attr, void *buf, size_t len, void *d),
		void *data);


/** @brief Set the value of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A bool value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_buffer_attr_write_bool(const struct iio_device *dev,
		const char *attr, bool val);


/** @brief Set the value of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A long long value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_buffer_attr_write_longlong(const struct iio_device *dev,
		const char *attr, long long val);


/** @brief Set the value of the given buffer-specific attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A double value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_buffer_attr_write_double(const struct iio_device *dev,
		const char *attr, double val);


/** @brief Associate a pointer to an iio_device structure
 * @param dev A pointer to an iio_device structure
 * @param data The pointer to be associated */
__api void iio_device_set_data(struct iio_device *dev, void *data);


/** @brief Retrieve a previously associated pointer of an iio_device structure
 * @param dev A pointer to an iio_device structure
 * @return The pointer previously associated if present, or NULL */
__api void * iio_device_get_data(const struct iio_device *dev);


/** @brief Retrieve the trigger of a given device
 * @param dev A pointer to an iio_device structure
 * @param trigger a pointer to a pointer of an iio_device structure. The pointed
 * pointer will be set to the address of the iio_device structure corresponding
 * to the associated trigger device.
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_get_trigger(const struct iio_device *dev,
		const struct iio_device **trigger);


/** @brief Associate a trigger to a given device
 * @param dev A pointer to an iio_device structure
 * @param trigger a pointer to the iio_device structure corresponding to the
 * trigger that should be associated.
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_set_trigger(const struct iio_device *dev,
		const struct iio_device *trigger);


/** @brief Return True if the given device is a trigger
 * @param dev A pointer to an iio_device structure
 * @return True if the device is a trigger, False otherwise */
__api __check_ret __pure bool iio_device_is_trigger(const struct iio_device *dev);

/** @brief Configure the number of kernel buffers for a device
 *
 * This function allows to change the number of buffers on kernel side.
 * @param dev A pointer to an iio_device structure
 * @param nb_buffers The number of buffers
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_set_kernel_buffers_count(const struct iio_device *dev,
		unsigned int nb_buffers);

/** @} *//* ------------------------------------------------------------------*/
/* ------------------------- Channel functions -------------------------------*/
/** @defgroup Channel Channel
 * @{
 * @struct iio_channel
 * @brief Represents an input or output channel of a device */


/** @brief Retrieve a pointer to the iio_device structure
 * @param chn A pointer to an iio_channel structure
 * @return A pointer to an iio_device structure */
__api __check_ret __pure const struct iio_device * iio_channel_get_device(
		const struct iio_channel *chn);


/** @brief Retrieve the channel ID (e.g. <b><i>voltage0</i></b>)
 * @param chn A pointer to an iio_channel structure
 * @return A pointer to a static NULL-terminated string */
__api __check_ret __pure const char * iio_channel_get_id(const struct iio_channel *chn);


/** @brief Retrieve the channel name (e.g. <b><i>vccint</i></b>)
 * @param chn A pointer to an iio_channel structure
 * @return A pointer to a static NULL-terminated string
 *
 * <b>NOTE:</b> if the channel has no name, NULL is returned. */
__api __check_ret __pure const char * iio_channel_get_name(const struct iio_channel *chn);


/** @brief Return True if the given channel is an output channel
 * @param chn A pointer to an iio_channel structure
 * @return True if the channel is an output channel, False otherwise */
__api __check_ret __pure bool iio_channel_is_output(const struct iio_channel *chn);


/** @brief Return True if the given channel is a scan element
 * @param chn A pointer to an iio_channel structure
 * @return True if the channel is a scan element, False otherwise
 *
 * <b>NOTE:</b> a channel that is a scan element is a channel that can
 * generate samples (for an input channel) or receive samples (for an output
 * channel) after being enabled. */
__api __check_ret __pure bool iio_channel_is_scan_element(const struct iio_channel *chn);


/** @brief Enumerate the channel-specific attributes of the given channel
 * @param chn A pointer to an iio_channel structure
 * @return The number of channel-specific attributes found */
__api __check_ret __pure unsigned int iio_channel_get_attrs_count(
		const struct iio_channel *chn);


/** @brief Get the channel-specific attribute present at the given index
 * @param chn A pointer to an iio_channel structure
 * @param index The index corresponding to the attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the index is invalid, NULL is returned */
__api __check_ret __pure const char * iio_channel_get_attr(
		const struct iio_channel *chn, unsigned int index);


/** @brief Try to find a channel-specific attribute by its name
 * @param chn A pointer to an iio_channel structure
 * @param name A NULL-terminated string corresponding to the name of the
 * attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the name does not correspond to any known attribute of the given
 * channel, NULL is returned
 *
 * <b>NOTE:</b> This function is useful to detect the presence of an attribute.
 * It can also be used to retrieve the name of an attribute as a pointer to a
 * static string from a dynamically allocated string. */
__api __check_ret __pure const char * iio_channel_find_attr(
		const struct iio_channel *chn, const char *name);


/** @brief Retrieve the filename of an attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr a NULL-terminated string corresponding to the name of the
 * attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the attribute name is unknown, NULL is returned */
__api __check_ret __pure const char * iio_channel_attr_get_filename(
		const struct iio_channel *chn, const char *attr);


/** @brief Read the content of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param dst A pointer to the memory area where the NULL-terminated string
 * corresponding to the value read will be stored
 * @param len The available length of the memory area, in bytes
 * @return On success, the number of bytes written to the buffer
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to iio_channel_attr_read,
 * it is now possible to read all of the attributes of a channel.
 *
 * The buffer is filled with one block of data per attribute of the channel,
 * by the order they appear in the iio_channel structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, it corresponds to the errno code that were
 * returned when reading the attribute; if positive, it corresponds to the
 * length of the data read. In that case, the rest of the block contains
 * the data. */
__api __check_ret ssize_t iio_channel_attr_read(const struct iio_channel *chn,
		const char *attr, char *dst, size_t len);


/** @brief Read the content of all channel-specific attributes
 * @param chn A pointer to an iio_channel structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the channel-specific attributes are read in one single
 * command. */
__api __check_ret int iio_channel_attr_read_all(struct iio_channel *chn,
		int (*cb)(struct iio_channel *chn,
			const char *attr, const char *val, size_t len, void *d),
		void *data);


/** @brief Read the content of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a bool variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_channel_attr_read_bool(const struct iio_channel *chn,
		const char *attr, bool *val);


/** @brief Read the content of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a long long variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_channel_attr_read_longlong(const struct iio_channel *chn,
		const char *attr, long long *val);


/** @brief Read the content of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A pointer to a double variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_channel_attr_read_double(const struct iio_channel *chn,
		const char *attr, double *val);


/** @brief Set the value of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param src A NULL-terminated string to set the attribute to
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to iio_channel_attr_write,
 * it is now possible to write all of the attributes of a channel.
 *
 * The buffer must contain one block of data per attribute of the channel,
 * by the order they appear in the iio_channel structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, the attribute is not written; if positive,
 * it corresponds to the length of the data to write. In that case, the rest
 * of the block must contain the data. */
__api __check_ret ssize_t iio_channel_attr_write(const struct iio_channel *chn,
		const char *attr, const char *src);


/** @brief Set the value of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param src A pointer to the data to be written
 * @param len The number of bytes that should be written
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned */
__api __check_ret ssize_t iio_channel_attr_write_raw(const struct iio_channel *chn,
		const char *attr, const void *src, size_t len);


/** @brief Set the values of all channel-specific attributes
 * @param chn A pointer to an iio_channel structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the channel-specific attributes are written in one single
 * command. */
__api __check_ret int iio_channel_attr_write_all(struct iio_channel *chn,
		ssize_t (*cb)(struct iio_channel *chn,
			const char *attr, void *buf, size_t len, void *d),
		void *data);


/** @brief Set the value of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A bool value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_channel_attr_write_bool(const struct iio_channel *chn,
		const char *attr, bool val);


/** @brief Set the value of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A long long value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_channel_attr_write_longlong(const struct iio_channel *chn,
		const char *attr, long long val);


/** @brief Set the value of the given channel-specific attribute
 * @param chn A pointer to an iio_channel structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * attribute
 * @param val A double value to set the attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_channel_attr_write_double(const struct iio_channel *chn,
		const char *attr, double val);


/** @brief Enable the given channel
 * @param chn A pointer to an iio_channel structure
 *
 * <b>NOTE:</b>Before creating an iio_buffer structure with
 * iio_device_create_buffer, it is required to enable at least one channel of
 * the device to read from. */
__api void iio_channel_enable(struct iio_channel *chn);


/** @brief Disable the given channel
 * @param chn A pointer to an iio_channel structure */
__api void iio_channel_disable(struct iio_channel *chn);


/** @brief Returns True if the channel is enabled
 * @param chn A pointer to an iio_channel structure
 * @return True if the channel is enabled, False otherwise */
__api __check_ret bool iio_channel_is_enabled(const struct iio_channel *chn);


/** @brief Demultiplex the samples of a given channel
 * @param chn A pointer to an iio_channel structure
 * @param buffer A pointer to an iio_buffer structure
 * @param dst A pointer to the memory area where the demultiplexed data will be
 * stored
 * @param len The available length of the memory area, in bytes
 * @return The size of the demultiplexed data, in bytes */
__api __check_ret size_t iio_channel_read_raw(const struct iio_channel *chn,
		struct iio_buffer *buffer, void *dst, size_t len);


/** @brief Demultiplex and convert the samples of a given channel
 * @param chn A pointer to an iio_channel structure
 * @param buffer A pointer to an iio_buffer structure
 * @param dst A pointer to the memory area where the converted data will be
 * stored
 * @param len The available length of the memory area, in bytes
 * @return The size of the converted data, in bytes */
__api __check_ret size_t iio_channel_read(const struct iio_channel *chn,
		struct iio_buffer *buffer, void *dst, size_t len);


/** @brief Multiplex the samples of a given channel
 * @param chn A pointer to an iio_channel structure
 * @param buffer A pointer to an iio_buffer structure
 * @param src A pointer to the memory area where the sequential data will
 * be read from
 * @param len The length of the memory area, in bytes
 * @return The number of bytes actually multiplexed */
__api __check_ret size_t iio_channel_write_raw(const struct iio_channel *chn,
		struct iio_buffer *buffer, const void *src, size_t len);


/** @brief Convert and multiplex the samples of a given channel
 * @param chn A pointer to an iio_channel structure
 * @param buffer A pointer to an iio_buffer structure
 * @param src A pointer to the memory area where the sequential data will
 * be read from
 * @param len The length of the memory area, in bytes
 * @return The number of bytes actually converted and multiplexed */
__api __check_ret size_t iio_channel_write(const struct iio_channel *chn,
		struct iio_buffer *buffer, const void *src, size_t len);


/** @brief Associate a pointer to an iio_channel structure
 * @param chn A pointer to an iio_channel structure
 * @param data The pointer to be associated */
__api void iio_channel_set_data(struct iio_channel *chn, void *data);


/** @brief Retrieve a previously associated pointer of an iio_channel structure
 * @param chn A pointer to an iio_channel structure
 * @return The pointer previously associated if present, or NULL */
__api void * iio_channel_get_data(const struct iio_channel *chn);


/** @brief Get the type of the given channel
 * @param chn A pointer to an iio_channel structure
 * @return The type of the channel */
__api __check_ret __pure enum iio_chan_type iio_channel_get_type(
		const struct iio_channel *chn);


/** @brief Get the modifier type of the given channel
 * @param chn A pointer to an iio_channel structure
 * @return The modifier type of the channel */
__api __check_ret __pure enum iio_modifier iio_channel_get_modifier(
		const struct iio_channel *chn);


/** @} *//* ------------------------------------------------------------------*/
/* ------------------------- Buffer functions --------------------------------*/
/** @defgroup Buffer Buffer
 * @{
 * @struct iio_buffer
 * @brief An input or output buffer, used to read or write samples */


/** @brief Retrieve a pointer to the iio_device structure
 * @param buf A pointer to an iio_buffer structure
 * @return A pointer to an iio_device structure */
__api __check_ret __pure const struct iio_device * iio_buffer_get_device(
		const struct iio_buffer *buf);


/** @brief Create an input or output buffer associated to the given device
 * @param dev A pointer to an iio_device structure
 * @param samples_count The number of samples that the buffer should contain
 * @param cyclic If True, enable cyclic mode
 * @return On success, a pointer to an iio_buffer structure
 * @return On error, NULL is returned, and errno is set to the error code
 *
 * <b>NOTE:</b> Channels that have to be written to / read from must be enabled
 * before creating the buffer. */
__api __check_ret struct iio_buffer * iio_device_create_buffer(const struct iio_device *dev,
		size_t samples_count, bool cyclic);


/** @brief Destroy the given buffer
 * @param buf A pointer to an iio_buffer structure
 *
 * <b>NOTE:</b> After that function, the iio_buffer pointer shall be invalid. */
__api void iio_buffer_destroy(struct iio_buffer *buf);

/** @brief Get a pollable file descriptor
 *
 * Can be used to know when iio_buffer_refill() or iio_buffer_push() can be
 * called
 * @param buf A pointer to an iio_buffer structure
 * @return On success, valid file descriptor
 * @return On error, a negative errno code is returned
 */
__api __check_ret int iio_buffer_get_poll_fd(struct iio_buffer *buf);

/** @brief Make iio_buffer_refill() and iio_buffer_push() blocking or not
 *
 * After this function has been called with blocking == false,
 * iio_buffer_refill() and iio_buffer_push() will return -EAGAIN if no data is
 * ready.
 * A device is blocking by default.
 * @param buf A pointer to an iio_buffer structure
 * @param blocking true if the buffer API should be blocking, else false
 * @return On success, 0
 * @return On error, a negative errno code is returned
 */
__api __check_ret int iio_buffer_set_blocking_mode(struct iio_buffer *buf, bool blocking);


/** @brief Fetch more samples from the hardware
 * @param buf A pointer to an iio_buffer structure
 * @return On success, the number of bytes read is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> Only valid for input buffers */
__api __check_ret ssize_t iio_buffer_refill(struct iio_buffer *buf);


/** @brief Send the samples to the hardware
 * @param buf A pointer to an iio_buffer structure
 * @return On success, the number of bytes written is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> Only valid for output buffers */
__api __check_ret ssize_t iio_buffer_push(struct iio_buffer *buf);


/** @brief Send a given number of samples to the hardware
 * @param buf A pointer to an iio_buffer structure
 * @param samples_count The number of samples to submit
 * @return On success, the number of bytes written is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> Only valid for output buffers */
__api __check_ret ssize_t iio_buffer_push_partial(struct iio_buffer *buf,
		size_t samples_count);

/** @brief Cancel all buffer operations
 * @param buf The buffer for which operations should be canceled
 *
 * This function cancels all outstanding buffer operations previously scheduled.
 * This means any pending iio_buffer_push() or iio_buffer_refill() operation
 * will abort and return immediately, any further invocations of these functions
 * on the same buffer will return immediately with an error.
 *
 * Usually iio_buffer_push() and iio_buffer_refill() will block until either all
 * data has been transferred or a timeout occurs. This can depending on the
 * configuration take a significant amount of time. iio_buffer_cancel() is
 * useful to bypass these conditions if the buffer operation is supposed to be
 * stopped in response to an external event (e.g. user input).
 *
 * To be able to capture additional data after calling this function the buffer
 * should be destroyed and then re-created.
 *
 * This function can be called multiple times for the same buffer, but all but
 * the first invocation will be without additional effect.
 *
 * This function is thread-safe, but not signal-safe, i.e. it must not be called
 * from a signal handler.
 */
__api void iio_buffer_cancel(struct iio_buffer *buf);


/** @brief Get the start address of the buffer
 * @param buf A pointer to an iio_buffer structure
 * @return A pointer corresponding to the start address of the buffer */
__api void * iio_buffer_start(const struct iio_buffer *buf);


/** @brief Find the first sample of a channel in a buffer
 * @param buf A pointer to an iio_buffer structure
 * @param chn A pointer to an iio_channel structure
 * @return A pointer to the first sample found, or to the end of the buffer if
 * no sample for the given channel is present in the buffer
 *
 * <b>NOTE:</b> This function, coupled with iio_buffer_step and iio_buffer_end,
 * can be used to iterate on all the samples of a given channel present in the
 * buffer, doing the following:
 *
 * @verbatim
 for (void *ptr = iio_buffer_first(buffer, chn); ptr < iio_buffer_end(buffer); ptr += iio_buffer_step(buffer)) {
    ....
 }
 @endverbatim */
__api void * iio_buffer_first(const struct iio_buffer *buf,
		const struct iio_channel *chn);


/** @brief Get the step size between two samples of one channel
 * @param buf A pointer to an iio_buffer structure
 * @return the difference between the addresses of two consecutive samples of
 * one same channel */
__api __check_ret ptrdiff_t iio_buffer_step(const struct iio_buffer *buf);


/** @brief Get the address that follows the last sample in a buffer
 * @param buf A pointer to an iio_buffer structure
 * @return A pointer corresponding to the address that follows the last sample
 * present in the buffer */
__api void * iio_buffer_end(const struct iio_buffer *buf);


/** @brief Call the supplied callback for each sample found in a buffer
 * @param buf A pointer to an iio_buffer structure
 * @param callback A pointer to a function to call for each sample found
 * @param data A user-specified pointer that will be passed to the callback
 * @return number of bytes processed.
 *
 * <b>NOTE:</b> The callback receives four arguments:
 * * A pointer to the iio_channel structure corresponding to the sample,
 * * A pointer to the sample itself,
 * * The length of the sample in bytes,
 * * The user-specified pointer passed to iio_buffer_foreach_sample. */
__api __check_ret ssize_t iio_buffer_foreach_sample(struct iio_buffer *buf,
		ssize_t (*callback)(const struct iio_channel *chn,
			void *src, size_t bytes, void *d), void *data);


/** @brief Associate a pointer to an iio_buffer structure
 * @param buf A pointer to an iio_buffer structure
 * @param data The pointer to be associated */
__api void iio_buffer_set_data(struct iio_buffer *buf, void *data);


/** @brief Retrieve a previously associated pointer of an iio_buffer structure
 * @param buf A pointer to an iio_buffer structure
 * @return The pointer previously associated if present, or NULL */
__api void * iio_buffer_get_data(const struct iio_buffer *buf);


/** @} *//* ------------------------------------------------------------------*/
/* ------------------------- Low-level functions -----------------------------*/
/** @defgroup Debug Debug and low-level functions
 * @{
 * @struct iio_data_format
 * @brief Contains the format of a data sample.
 *
 * The different fields inform about the correct way to convert one sample from
 * its raw format (as read from / generated by the hardware) to its real-world
 * value.
 */
struct iio_data_format {
	/** @brief Total length of the sample, in bits */
	unsigned int length;

	/** @brief Length of valuable data in the sample, in bits */
	unsigned int bits;

	/** @brief Right-shift to apply when converting sample */
	unsigned int shift;

	/** @brief Contains True if the sample is signed */
	bool is_signed;

	/** @brief Contains True if the sample is fully defined, sign extended, etc. */
	bool is_fully_defined;

	/** @brief Contains True if the sample is in big-endian format */
	bool is_be;

	/** @brief Contains True if the sample should be scaled when converted */
	bool with_scale;

	/** @brief Contains the scale to apply if with_scale is set */
	double scale;

	/** @brief Number of times length repeats (added in v0.8) */
	unsigned int repeat;
};


/** @brief Get the current sample size
 * @param dev A pointer to an iio_device structure
 * @return On success, the sample size in bytes
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> The sample size is not constant and will change when channels
 * get enabled or disabled. */
__api __check_ret ssize_t iio_device_get_sample_size(const struct iio_device *dev);


/** @brief Get the index of the given channel
 * @param chn A pointer to an iio_channel structure
 * @return On success, the index of the specified channel
 * @return On error, a negative errno code is returned */
__api __check_ret __pure long iio_channel_get_index(const struct iio_channel *chn);


/** @brief Get a pointer to a channel's data format structure
 * @param chn A pointer to an iio_channel structure
 * @return A pointer to the channel's iio_data_format structure */
__api __check_ret __cnst const struct iio_data_format * iio_channel_get_data_format(
		const struct iio_channel *chn);


/** @brief Convert the sample from hardware format to host format
 * @param chn A pointer to an iio_channel structure
 * @param dst A pointer to the destination buffer where the converted sample
 * should be written
 * @param src A pointer to the source buffer containing the sample */
__api void iio_channel_convert(const struct iio_channel *chn,
		void *dst, const void *src);


/** @brief Convert the sample from host format to hardware format
 * @param chn A pointer to an iio_channel structure
 * @param dst A pointer to the destination buffer where the converted sample
 * should be written
 * @param src A pointer to the source buffer containing the sample */
__api void iio_channel_convert_inverse(const struct iio_channel *chn,
		void *dst, const void *src);


/** @brief Enumerate the debug attributes of the given device
 * @param dev A pointer to an iio_device structure
 * @return The number of debug attributes found */
__api __check_ret __pure unsigned int iio_device_get_debug_attrs_count(
		const struct iio_device *dev);


/** @brief Get the debug attribute present at the given index
 * @param dev A pointer to an iio_device structure
 * @param index The index corresponding to the debug attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the index is invalid, NULL is returned */
__api __check_ret __pure const char * iio_device_get_debug_attr(
		const struct iio_device *dev, unsigned int index);


/** @brief Try to find a debug attribute by its name
 * @param dev A pointer to an iio_device structure
 * @param name A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @return On success, a pointer to a static NULL-terminated string
 * @return If the name does not correspond to any known debug attribute of the
 * given device, NULL is returned
 *
 * <b>NOTE:</b> This function is useful to detect the presence of a debug
 * attribute.
 * It can also be used to retrieve the name of a debug attribute as a pointer
 * to a static string from a dynamically allocated string. */
__api __check_ret __pure const char * iio_device_find_debug_attr(
		const struct iio_device *dev, const char *name);


/** @brief Read the content of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param dst A pointer to the memory area where the NULL-terminated string
 * corresponding to the value read will be stored
 * @param len The available length of the memory area, in bytes
 * @return On success, the number of bytes written to the buffer
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to
 * iio_device_debug_attr_read, it is now possible to read all of the debug
 * attributes of a device.
 *
 * The buffer is filled with one block of data per debug attribute of the
 * device, by the order they appear in the iio_device structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, it corresponds to the errno code that were
 * returned when reading the debug attribute; if positive, it corresponds
 * to the length of the data read. In that case, the rest of the block contains
 * the data. */
__api __check_ret ssize_t iio_device_debug_attr_read(const struct iio_device *dev,
		const char *attr, char *dst, size_t len);


/** @brief Read the content of all debug attributes
 * @param dev A pointer to an iio_device structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the debug attributes are read in one single command. */
__api __check_ret int iio_device_debug_attr_read_all(struct iio_device *dev,
		int (*cb)(struct iio_device *dev, const char *attr,
			const char *value, size_t len, void *d),
		void *data);


/** @brief Set the value of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param src A NULL-terminated string to set the debug attribute to
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b>By passing NULL as the "attr" argument to
 * iio_device_debug_attr_write, it is now possible to write all of the
 * debug attributes of a device.
 *
 * The buffer must contain one block of data per debug attribute of the device,
 * by the order they appear in the iio_device structure.
 *
 * The first four bytes of one block correspond to a 32-bit signed value in
 * network order. If negative, the debug attribute is not written; if positive,
 * it corresponds to the length of the data to write. In that case, the rest
 * of the block must contain the data. */
__api __check_ret ssize_t iio_device_debug_attr_write(const struct iio_device *dev,
		const char *attr, const char *src);


/** @brief Set the value of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param src A pointer to the data to be written
 * @param len The number of bytes that should be written
 * @return On success, the number of bytes written
 * @return On error, a negative errno code is returned */
__api __check_ret ssize_t iio_device_debug_attr_write_raw(const struct iio_device *dev,
		const char *attr, const void *src, size_t len);


/** @brief Set the values of all debug attributes
 * @param dev A pointer to an iio_device structure
 * @param cb A pointer to a callback function
 * @param data A pointer that will be passed to the callback function
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned
 *
 * <b>NOTE:</b> This function is especially useful when used with the network
 * backend, as all the debug attributes are written in one single command. */
__api __check_ret int iio_device_debug_attr_write_all(struct iio_device *dev,
		ssize_t (*cb)(struct iio_device *dev,
			const char *attr, void *buf, size_t len, void *d),
		void *data);


/** @brief Read the content of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param val A pointer to a bool variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_debug_attr_read_bool(const struct iio_device *dev,
		const char *attr, bool *val);


/** @brief Read the content of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param val A pointer to a long long variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_debug_attr_read_longlong(const struct iio_device *dev,
		const char *attr, long long *val);


/** @brief Read the content of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param val A pointer to a double variable where the value should be stored
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_debug_attr_read_double(const struct iio_device *dev,
		const char *attr, double *val);


/** @brief Set the value of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param val A bool value to set the debug attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_debug_attr_write_bool(const struct iio_device *dev,
		const char *attr, bool val);


/** @brief Set the value of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param val A long long value to set the debug attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_debug_attr_write_longlong(const struct iio_device *dev,
		const char *attr, long long val);


/** @brief Set the value of the given debug attribute
 * @param dev A pointer to an iio_device structure
 * @param attr A NULL-terminated string corresponding to the name of the
 * debug attribute
 * @param val A double value to set the debug attribute to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_debug_attr_write_double(const struct iio_device *dev,
		const char *attr, double val);


/** @brief Identify the channel or debug attribute corresponding to a filename
 * @param dev A pointer to an iio_device structure
 * @param filename A NULL-terminated string corresponding to the filename
 * @param chn A pointer to a pointer of an iio_channel structure. The pointed
 * pointer will be set to the address of the iio_channel structure if the
 * filename correspond to the attribute of a channel, or NULL otherwise.
 * @param attr A pointer to a NULL-terminated string. The pointer
 * pointer will be set to point to the name of the attribute corresponding to
 * the filename.
 * @return On success, 0 is returned, and *chn and *attr are modified.
 * @return On error, a negative errno code is returned. *chn and *attr are not
 * modified. */
__api __check_ret int iio_device_identify_filename(const struct iio_device *dev,
		const char *filename, struct iio_channel **chn,
		const char **attr);


/** @brief Set the value of a hardware register
 * @param dev A pointer to an iio_device structure
 * @param address The address of the register
 * @param value The value to set the register to
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_reg_write(struct iio_device *dev,
		uint32_t address, uint32_t value);


/** @brief Get the value of a hardware register
 * @param dev A pointer to an iio_device structure
 * @param address The address of the register
 * @param value A pointer to the variable where the value will be written
 * @return On success, 0 is returned
 * @return On error, a negative errno code is returned */
__api __check_ret int iio_device_reg_read(struct iio_device *dev,
		uint32_t address, uint32_t *value);


/** @} */

#ifdef __cplusplus
}
#endif

#undef __api

#endif /* __IIO_H__ */
