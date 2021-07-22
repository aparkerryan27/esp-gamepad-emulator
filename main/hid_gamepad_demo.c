
#define BTSTACK_FILE__ "hid_gamepad_demo.c"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "btstack.h"

// from USB HID Specification 1.11, HID Report Descriptor
// https://www.usb.org/sites/default/files/documents/hid1_11.pdf
/*
const uint8_t hid_descriptor_gamepad[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    //0xA1, 0x00,        //   COLLECTION (Physical)

        //Sticks
        0x09, 0x30,        //   Usage (X)
        0x09, 0x31,        //   Usage (Y)
        0x15, 0x00,        //   Logical Minimum (0)
        0x26, 0xFF, 0x00,  //   Logical Maximum (255)
                            
                           //   Two values that can be returned at the same time up to 8 bits in size
        0x75, 0x08,        //   Report Size (8)
        0x95, 0x02,        //   Report Count (2)
        0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
                           //   0x02 hex -> binary flags 0100 0000
    
        //Buttons
        0x19, 0x01,        //   Usage Minimum (0x01)
        0x29, 0x08,        //   Usage Maximum (0x08)
        0x15, 0x00,        //   Logical Minimum (0)
        0x25, 0x01,        //   Logical Maximum (1)

                           // 8 values of 1 bit each (on off)
        0x75, 0x01,        //   Report Size (1)
        0x95, 0x08,        //   Report Count (8)
        0x81, 0x02,        //   Input (Data,Var,Abs)...


                    
        //0xc0, //End Physical Collection
  
    0xc0 //End Application Collection
};
*/

const uint8_t hid_descriptor_gamepad[] = {
    0x05, 0x01,  /* USAGE_PAGE (Generic Desktop)       */
    0x09, 0x05,  /* USAGE (Game Pad)                   */
    0xa1, 0x01,  /* COLLECTION (Application)           */
    0xa1, 0x03,  /*   COLLECTION (Report)              */

    0x85, 0x01,  /*     REPORT_ID (1)                  */

    0x05, 0x09,  /*     USAGE_PAGE (Button)            */
    0x19, 0x01,  /*     USAGE_MINIMUM (Button 1)       */
    0x29, 0x08,  /*     USAGE_MAXIMUM (Button 8)      */
    0x15, 0x00,  /*     LOGICAL_MINIMUM (0)            */
    0x25, 0x01,  /*     LOGICAL_MAXIMUM (1)            */
    0x95, 0x08,  /*     REPORT_COUNT (8)              */
    0x75, 0x01,  /*     REPORT_SIZE (1)                */
    0x81, 0x02,  /*     INPUT (Data,Var,Abs)           */

    0xa1, 0x00,  /*     COLLECTION (Physical)          */
    0x05, 0x01,  /*       USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,  /*       USAGE (X)                    */
    0x09, 0x31,  /*       USAGE (Y)                    */
    0x15, 0x00,  /*       LOGICAL_MINIMUM (0)          */
    0x25, 0x64,  /*       LOGICAL_MAXIMUM (100)        */
    0x75, 0x08,  /*       REPORT_SIZE (8)              */
    0x95, 0x02,  /*       REPORT_COUNT (2)             */
    0x81, 0x02,  /*       INPUT (Data,Var,Abs)         */
    0xc0,        /*     END_COLLECTION                 */

    0xc0,        /*   END_COLLECTION                   */
    0xc0         /* END_COLLECTION                     */
};
// STATE

static uint8_t hid_service_buffer[250];
static uint8_t device_id_sdp_service_buffer[100];
static const char hid_device_name[] = "";
static btstack_packet_callback_registration_t hci_event_callback_registration;
static uint16_t hid_cid;
static bd_addr_t device_addr;
static uint8_t hid_boot_device = 0;

static enum {
    APP_BOOTING,
    APP_NOT_CONNECTED,
    APP_CONNECTING,
    APP_CONNECTED
} app_state = APP_BOOTING;

// HID Report sending
static uint8_t send_m1;
static uint8_t send_m2;
static uint8_t send_buttons; //TODO: - set this back to all zero? use |= button_number to set button active

static void send_key(uint8_t m1, uint8_t m2, uint8_t buttons){
    send_m1 = m1;
    send_m2 = m2;
    send_buttons = buttons;
    hid_device_request_can_send_now_event(hid_cid);
}

static void send_report(uint8_t m1, uint8_t m2, uint8_t buttons){


    //TODO: i am the most unsure about this report.
        //do i need padding??
        //Error Codes I keep recieving 
            //0x04 - ERROR_CODE_PAGE_TIMEOUT
            //0x0c - CONNECTION_DISALLOWED
            //0x69 - L2CAP_CONNECTION_RESPONSE_RESULT_RTX_TIMEOUT 
    uint8_t report[] =   {buttons, m1, m2}; 
    hid_device_send_interrupt_message(hid_cid, &report[0], sizeof(report));
}

static void write_send_input(uint8_t m1, uint8_t m2, uint8_t buttons){
    switch (app_state){
        case APP_BOOTING:
        case APP_CONNECTING:
            // ignore
            break;

        case APP_CONNECTED:
            // send values
            send_key(m1, m2, buttons);
            return;
      
        case APP_NOT_CONNECTED:
            printf("Connecting to %s...\n", bd_addr_to_str(device_addr));
            hid_device_connect(device_addr, &hid_cid);
            break;
        default:
            btstack_assert(false);
            break;
    }
}

#define CHANGE_PERIOD_MS 15

static int step;
static const int STEPS_PER_CYCLE = 50;

static btstack_timer_source_t gaming_timer;

static void gaming_timer_handler(btstack_timer_source_t * ts){

    //blocking statement for initiation of HID connection
    if (!hid_cid) return;

    //tap all the buttons on or off every full rotation
    if (step % STEPS_PER_CYCLE == 0){
        
        send_buttons |= 1;\

        send_m1 = 0xF0;
        send_m2 = 0x1F;
    } else {
        send_m1 = 0;
        send_m2 = 0b11111111; //0xFF 256 max int8
    }
  
    // next
    step++;
    if (step >= STEPS_PER_CYCLE * 4) {
        step = 0;
    }

    // trigger send
    write_send_input(send_m1, send_m2, send_buttons);

    // set next timer
    btstack_run_loop_set_timer(ts, CHANGE_PERIOD_MS);
    btstack_run_loop_add_timer(ts);
}

static void hid_simulate_actions(void){
    printf("Start toggling gamepad keys..\n");

    step = 0;

    // set one-shot timer
    gaming_timer.process = &gaming_timer_handler;
    btstack_run_loop_set_timer(&gaming_timer, CHANGE_PERIOD_MS);
    btstack_run_loop_add_timer(&gaming_timer);
}




static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t * packet, uint16_t packet_size){
    UNUSED(channel);
    UNUSED(packet_size);
    uint8_t status;
    switch (packet_type){
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)){
                case BTSTACK_EVENT_STATE:
                    if (btstack_event_state_get_state(packet) != HCI_STATE_WORKING) return;
                    app_state = APP_NOT_CONNECTED;
                    break;

                case HCI_EVENT_USER_CONFIRMATION_REQUEST:
                    // ssp: inform about user confirmation request
                    log_info("SSP User Confirmation Request with numeric value '%06"PRIu32"'\n", hci_event_user_confirmation_request_get_numeric_value(packet));
                    log_info("SSP User Confirmation Auto accept\n");                   
                    break; 

                case HCI_EVENT_HID_META:
                    switch (hci_event_hid_meta_get_subevent_code(packet)){
                        case HID_SUBEVENT_CONNECTION_OPENED:
                            status = hid_subevent_connection_opened_get_status(packet);
                            if (status != ERROR_CODE_SUCCESS) {
                                // outgoing connection failed
                                printf("Connection failed, status 0x%x\n", status);
                                app_state = APP_NOT_CONNECTED;
                                hid_cid = 0;
                                return;
                            }
                            app_state = APP_CONNECTED;
                            hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);
                            printf("HID Connected, sending demo program...\n");
                            hid_simulate_actions();
    
                            break;
                        case HID_SUBEVENT_CONNECTION_CLOSED:
                            printf("HID Disconnected\n");
                            app_state = APP_NOT_CONNECTED;
                            hid_cid = 0;
                            break;
                        case HID_SUBEVENT_CAN_SEND_NOW:
                            //TODO: in conjunction with the todo below this, this if is definitely not valid for ints
                            if ((send_m1) && (send_m2)){
                                send_report(send_m1, send_m2, send_buttons);
                                //TODO: - should i be setting these back to zero here or keeping their values? 
                                send_m1 = 0;
                                send_m2 = 0;
                                send_buttons = 0;
                                hid_device_request_can_send_now_event(hid_cid);
                            } else {
                                //send a 0 value if the variables have not been set yet but the connection needs to send information
                                log_info("Values not available - sending a null value");
                                send_report(0, 0, 0);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

/* 
 * To run a HID Device service you need to initialize the SDP, and to create and register HID Device record with it. 
 * At the end the Bluetooth stack is started.
 */
int btstack_main(int argc, const char * argv[]){
    (void)argc;
    (void)argv;

    // allow to get found by inquiry
    gap_discoverable_control(1);
    // use Limited Discoverable Mode; Peripheral; Gamepad as CoD
    gap_set_class_of_device(0x2508);
    // set local name to be identified - zeroes will be replaced by actual BD ADDR
    gap_set_local_name("PEGamepad2");
    // allow for role switch in general and sniff mode
    gap_set_default_link_policy_settings( LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE );
    // allow for role switch on outgoing connections - this allow HID Host to become master when we re-connect to it
    gap_set_allow_role_switch(true);

    // L2CAP
    l2cap_init();

    // SDP Server
    sdp_init();
    memset(hid_service_buffer, 0, sizeof(hid_service_buffer));

    uint8_t hid_virtual_cable = 1;
    uint8_t hid_remote_wake = 1;
    uint8_t hid_reconnect_initiate = 1;
    uint8_t hid_normally_connectable = 1;

    hid_sdp_record_t hid_params = {
        // hid service subclass Gamepad, hid counntry code 33 US
        //http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html
        0x2508, 33, //subclass, country code

        hid_virtual_cable, hid_remote_wake, 
        hid_reconnect_initiate, hid_normally_connectable,
        hid_boot_device, 
        0xFFFF, 0xFFFF, 3200, //hid_ssr__host_max_latency,  hid_ssr_host_min_timeout, hid_supervision_timeout
        hid_descriptor_gamepad,
        sizeof(hid_descriptor_gamepad), 
        hid_device_name
    };

    
    hid_create_sdp_record(hid_service_buffer, 0x10001, &hid_params);

    printf("HID service record size: %u\n", de_get_len( hid_service_buffer));
    sdp_register_service(hid_service_buffer);

    // See https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers if you don't have a USB Vendor ID and need a Bluetooth Vendor ID
    // device info: BlueKitchen GmbH, product 1, version 1
    device_id_create_sdp_record(device_id_sdp_service_buffer, 0x10003, DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH, BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH, 1, 1);
    printf("Device ID SDP service record size: %u\n", de_get_len((uint8_t*)device_id_sdp_service_buffer));
    sdp_register_service(device_id_sdp_service_buffer);

    // HID Device
    hid_device_init(hid_boot_device, sizeof(hid_descriptor_gamepad), hid_descriptor_gamepad);
       
    // register for HCI events
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // register for HID events
    hid_device_register_packet_handler(&packet_handler);

    // turn on!
    hci_power_control(HCI_POWER_ON);

    return 0;
}