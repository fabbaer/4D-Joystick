/*******************************************************************************
* @file         : buttons.c
* @project      : 4D-Joystick, Joystick-Unit
* @author       : Fabian Baer
* @brief        : Handles debouncing of the buddybuttons and rotary encoder
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <cmsis_os.h>
#include <stm32l4xx_hal.h>
#include <board.h>
#include <system.h>
#include <buttons.h>


/* Typedefs ------------------------------------------------------------------*/
typedef enum {
  btnFSM_risingEdge,
  btnFSM_high_hold,
  btnFSM_high,
  btnFSM_fallingEdge,
  btnFSM_low_hold,
  btnFSM_low
} ButtonFSM_t;

typedef enum {
  btnState_fallingEdge,
  btnState_low,
  btnState_risingEdge,
  btnState_high
} ButtonState_t;

typedef struct {
  ButtonFSM_t statemachine;
  ButtonState_t state;
  uint32_t timeout;
  uint32_t holdTime;
  GPIO_TypeDef* port;
  uint16_t pin;
} Button_t;

//Check if Message-Unions have the size of an uint32_t
typedef uint8_t assertBuddyMsgSize[(sizeof(BuddyButtonMessage_t)==4)*2-1 ];
typedef uint8_t assertRotaryMsgSize[(sizeof(RotaryEncoderMessage_t)==4)*2-1 ];


/* Prototypes ----------------------------------------------------------------*/
static void buttons_task( void const *argument );
static void buttons_initStruct( Button_t* pBtn, uint32_t holdTime, GPIO_TypeDef* port, uint16_t pin );
static void buttons_statemachine( Button_t* pBtn );
static void buttons_handleBuddyButton( Button_t* pBtn, Config_BuddyButton_t buddyId );
static inline void buttons_handleRotaryEncoder( Button_t* pRot1, Button_t* pRot2, Button_t* pRotP );


/* Variables -----------------------------------------------------------------*/
osThreadId htask_buttons;
osMessageQDef(buddyButtonsMsgBox, 32, BuddyButtonMessage_t);
osMessageQId buddyButtonsMsgBox;
osMessageQDef(rotaryEncoderMsgBox, 32, RotaryEncoderMessage_t);
osMessageQId rotaryEncoderMsgBox;
extern uint32_t system_watchdog_buttonTask;

/* Code ----------------------------------------------------------------------*/

/*******************************************************************************
 * Initializes all GPIOs and message queues for the buttons.
 *
 * @return nothing
 *******************************************************************************/
void buttons_init( void ) {
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  GPIO_InitStruct.Pin = BB_4_Pin | BB_3_Pin | BB_2_Pin | BB_1_Pin | BTN3_Pin
      | BTN2_Pin | BTN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  buddyButtonsMsgBox = osMessageCreate(osMessageQ(buddyButtonsMsgBox), NULL);
  rotaryEncoderMsgBox = osMessageCreate(osMessageQ(rotaryEncoderMsgBox), NULL);
}


/*******************************************************************************
 * Returns the handle of the buddybutton message queue.
 *
 * @return buddybutton message queue
 *******************************************************************************/
osMessageQId buttons_getBuddyButtonsMQ( void ) {
  return buddyButtonsMsgBox;
}


/*******************************************************************************
 * Returns the handle of the rotary encoder message queue.
 *
 * @return rotary encoder message queue
 *******************************************************************************/
osMessageQId buttons_getRotaryEncoderMQ( void ) {
  return rotaryEncoderMsgBox;
}


/*******************************************************************************
 * Initializes the task.
 *
 * @param priority The task priority
 * @return nothing
 *******************************************************************************/
void buttons_setupTask( osPriority priority ) {
  osThreadDef(buttonsTask, buttons_task, priority, 0, 512);
  htask_buttons = osThreadCreate(osThread(buttonsTask), NULL);
}


/*******************************************************************************
 * The buttons-task.
 * This task handles the buttons statemachine, which is responsible for
 * debouncing and edge detection.
 *
 * @return nothing
 *******************************************************************************/
static void buttons_task( void const *argument ) {
  (void)argument;
  Button_t bb1, bb2, bb3, bb4, rotP, rot1, rot2;

  //Initialize structs
  buttons_initStruct(&bb1, 50, BB_1_Port, BB_1_Pin);
  buttons_initStruct(&bb2, 50, BB_2_Port, BB_2_Pin);
  buttons_initStruct(&bb3, 50, BB_3_Port, BB_3_Pin);
  buttons_initStruct(&bb4, 50, BB_4_Port, BB_4_Pin);
  buttons_initStruct(&rot1, 1, BTN2_Port, BTN2_Pin);
  buttons_initStruct(&rot2, 1, BTN3_Port, BTN3_Pin);
  buttons_initStruct(&rotP, 1, BTN1_Port, BTN1_Pin);

  while(1) {
    buttons_handleBuddyButton(&bb1, buddyButton1);
    buttons_handleBuddyButton(&bb2, buddyButton2);
    buttons_handleBuddyButton(&bb3, buddyButton3);
    buttons_handleBuddyButton(&bb4, buddyButton4);

    buttons_handleRotaryEncoder(&rot1, &rot2, &rotP);

    system_watchdog_buttonTask++;
    osDelay(2);
  }
}


/*******************************************************************************
 * Initializes the button-struct with default values.
 *
 * @param pBtn A pointer to the used button-struct
 * @param holdTime Time where transitions are blocked, after a transition
 * @param port The port of the button GPIO
 * @param pin The pin of the button GPIO
 * @return nothing
 *******************************************************************************/
static void buttons_initStruct( Button_t* pBtn, uint32_t holdTime, GPIO_TypeDef* port, uint16_t pin ) {
  pBtn->statemachine = btnFSM_high;
  pBtn->timeout = 0;
  pBtn->holdTime = holdTime;
  pBtn->port = port;
  pBtn->pin = pin;
}


/*******************************************************************************
 * The button-statemachine.
 * A state transition (high->low, low->high) is valid, when it last for at
 * least two cycles. On the second cycle the state changes to
 * btnState_risingEdge/btnState_fallingEdge for one cycle.
 * No new state transitions are possible for "holdTime" milliseconds after a
 * state transition occurres.
 *
 * @param pBtn A pointer to the used button-struct
 * @return nothing
 *******************************************************************************/
static void buttons_statemachine( Button_t* pBtn ) {
  bool pinState = (HAL_GPIO_ReadPin(pBtn->port, pBtn->pin) == GPIO_PIN_SET) ? true : false;

  switch(pBtn->statemachine) {
    case btnFSM_low:
      if(pinState) {
        pBtn->statemachine = btnFSM_risingEdge;
      }
      pBtn->state = btnState_low;
      break;

    case btnFSM_risingEdge:
      if(pinState) {
        pBtn->statemachine = btnFSM_high_hold;
        pBtn->timeout = HAL_GetTick() + pBtn->holdTime;
        pBtn->state = btnState_risingEdge;
      } else {
        pBtn->statemachine = btnFSM_low;
        pBtn->state = btnState_low;
      }
      break;

    case btnFSM_high_hold:
      if(HAL_GetTick() > pBtn->timeout) {
        pBtn->statemachine = btnFSM_high;
      }
      pBtn->state = btnState_high;
      break;

    case btnFSM_high:
      if(!pinState) {
        pBtn->statemachine = btnFSM_fallingEdge;
      }
      pBtn->state = btnState_high;
      break;

    case btnFSM_fallingEdge:
      if(pinState) {
        pBtn->statemachine = btnFSM_high;
        pBtn->state = btnState_high;
      } else {
        pBtn->statemachine = btnFSM_low_hold;
        pBtn->timeout = HAL_GetTick() + pBtn->holdTime;
        pBtn->state = btnState_fallingEdge;
      }
      break;

    case btnFSM_low_hold:
      if(HAL_GetTick() > pBtn->timeout) {
        pBtn->statemachine = btnFSM_low;
      }
      pBtn->state = btnState_low;
      break;

    default:
      pBtn->statemachine = btnFSM_high;
      pBtn->timeout = 0;
      pBtn->state = btnState_high;
      break;
  }
}


/*******************************************************************************
 * Handles buttybuttons state transitions and creates messages for theme.
 *
 * @param pBtn A pointer to the used button-struct
 * @param pin The current button
 * @return nothing
 *******************************************************************************/
static void buttons_handleBuddyButton( Button_t* pBtn, Config_BuddyButton_t buddyId ) {
  BuddyButtonMessage_t msg;
  msg.fields.event = buddybutton_none;

  //Update state
  buttons_statemachine(pBtn);

  //Check for state changes and create message
  if(pBtn->state == btnState_fallingEdge) {
    msg.fields.buddyId = buddyId;
    msg.fields.event = buddybutton_pressed;
  } else if(pBtn->state == btnState_risingEdge) {
    msg.fields.buddyId = buddyId;
    msg.fields.event = buddybutton_released;
  }

  //Send message if required, discard message if queue is full
  if(msg.fields.event != buddybutton_none) {
    if(osMessageAvailableSpace(buddyButtonsMsgBox) > 0) {
      osMessagePut(buddyButtonsMsgBox, msg.bits, osWaitForever);
    }
  }
}


/*******************************************************************************
 * Handles rotary encoder state transitions and creates messages for theme.
 *
 * @param pRot1 A pointer to the button-struct (rotary channel 1)
 * @param pRot2 A pointer to the button-struct (rotary channel 2)
 * @param pRotP A pointer to the button-struct (momentary pushbutton)
 * @return nothing
 *******************************************************************************/
static inline void buttons_handleRotaryEncoder( Button_t* pRot1, Button_t* pRot2, Button_t* pRotP ) {
  RotaryEncoderMessage_t msg;
  msg.event = rotaryEncoder_none;

  //Update states
  buttons_statemachine(pRot1);
  buttons_statemachine(pRot2);
  buttons_statemachine(pRotP);

  //Check for state changes and create message (pushbutton)
  if(pRotP->state == btnState_fallingEdge) {
    msg.event = rotaryEncoder_pressed;
  } else if(pRotP->state == btnState_risingEdge) {
    msg.event = rotaryEncoder_released;
  }

  //Check for state changes and create message (rotary encoder)
  if(pRot1->state == btnState_fallingEdge) {
    if(pRot2->state == btnState_high) {
      msg.event = rotaryEncoder_clockwise;
    } else {
      msg.event = rotaryEncoder_counterclockwise;
    }
  }

  //Send message if required, discard message if queue is full
  if(msg.event != rotaryEncoder_none) {
    if(osMessageAvailableSpace(rotaryEncoderMsgBox) > 0) {
      osMessagePut(rotaryEncoderMsgBox, msg.bits, osWaitForever);
    }
  }
}
