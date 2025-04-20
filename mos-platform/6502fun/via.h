#ifndef VIA6522_H
#define VIA6522_H

#include <stdint.h> // For uint8_t and uint16_t
#include <stdbool.h> // For bool

// --- VIA Base Address ---
#define VIA_BASE    0x6000

// --- VIA Register Offsets ---
// These remain internal details, but are useful for the implementation
#define VIA_ORB_REG     0x00 // Port B Output Register (Write) / Input Register (Read)
#define VIA_ORA_REG     0x01 // Port A Output Register (Write) / Input Register w/ Handshake (Read)
#define VIA_DDRB_REG    0x02 // Data Direction Register B
#define VIA_DDRA_REG    0x03 // Data Direction Register A
#define VIA_T1CL_REG    0x04 // Timer 1 Counter Low (Read) / Latch Low (Write)
#define VIA_T1CH_REG    0x05 // Timer 1 Counter High (Read) / Latch High (Write)
#define VIA_T1LL_REG    0x06 // Timer 1 Latch Low (Read)
#define VIA_T1LH_REG    0x07 // Timer 1 Latch High (Read)
// #define VIA_T2CL_REG    0x08 // Timer 2 Counter Low (Read) / Latch Low (Write) - RESERVED FOR SYSTICK
// #define VIA_T2CH_REG    0x09 // Timer 2 Counter High (Read) / Latch High (Write) - RESERVED FOR SYSTICK
#define VIA_SR_REG      0x0A // Shift Register
#define VIA_ACR_REG     0x0B // Auxiliary Control Register
#define VIA_PCR_REG     0x0C // Peripheral Control Register
#define VIA_IFR_REG     0x0D // Interrupt Flag Register
#define VIA_IER_REG     0x0E // Interrupt Enable Register
#define VIA_ORA_NHS_REG 0x0F // Port A Input Register No Handshake (Read)

// --- Internal Helper Macro for Register Access ---
/**
 * @brief Wraps a memory address cast to a volatile uint8_t pointer for direct access.
 * Using volatile ensures the compiler doesn't optimize away reads/writes to hardware.
 */
#define VIA_REG(offset) (*(volatile uint8_t*)(VIA_BASE + (offset)))

// --- Constants and Bit Definitions (Internal, used by implementation) ---
// ACR bits
#define VIA_ACR_PA_LATCH_EN_BIT 0x01
#define VIA_ACR_PB_LATCH_EN_BIT 0x02
#define VIA_ACR_SR_MODE_SHIFT   2
#define VIA_ACR_SR_MODE_MASK    (0x07 << VIA_ACR_SR_MODE_SHIFT)
#define VIA_ACR_T2_MODE_BIT     0x20 // RESERVED FOR SYSTICK
#define VIA_ACR_T1_MODE_SHIFT   6
#define VIA_ACR_T1_MODE_MASK    (0x03 << VIA_ACR_T1_MODE_SHIFT)

// PCR bits
#define VIA_PCR_CA1_MODE_SHIFT  0
#define VIA_PCR_CA1_MODE_MASK   (0x01 << VIA_PCR_CA1_MODE_SHIFT) // Edge select only
#define VIA_PCR_CA2_MODE_SHIFT  1
#define VIA_PCR_CA2_MODE_MASK   (0x07 << VIA_PCR_CA2_MODE_SHIFT)
#define VIA_PCR_CB1_MODE_SHIFT  4
#define VIA_PCR_CB1_MODE_MASK   (0x01 << VIA_PCR_CB1_MODE_SHIFT) // Edge select only
#define VIA_PCR_CB2_MODE_SHIFT  5
#define VIA_PCR_CB2_MODE_MASK   (0x07 << VIA_PCR_CB2_MODE_SHIFT)

// IFR/IER bits
#define VIA_INT_CA1_BIT         0x01
#define VIA_INT_CA2_BIT         0x02
#define VIA_INT_CB1_BIT         0x04
#define VIA_INT_CB2_BIT         0x08
#define VIA_INT_SR_BIT          0x10
#define VIA_INT_T2_BIT          0x20 // RESERVED FOR SYSTICK
#define VIA_INT_T1_BIT          0x40
#define VIA_INT_IRQ_BIT         0x80 // Read-only in IFR, Set/Clear control in IER

// --- Public Enums for Configuration ---

typedef enum {
    VIA_PIN_INPUT = 0,
    VIA_PIN_OUTPUT = 1
} ViaPinMode;

typedef enum {
    VIA_PORT_A = 0,
    VIA_PORT_B = 1
} ViaPort;

typedef enum {
    VIA_TIMER1_CONTINUOUS_INTERRUPT = 0x00, // Generate continuous interrupts on T1 underflow
    VIA_TIMER1_CONTINUOUS_PB7_PULSE = 0x40, // Generate continuous square wave on PB7
    VIA_TIMER1_ONE_SHOT_INTERRUPT   = 0x80, // Generate one interrupt on T1 underflow
    VIA_TIMER1_ONE_SHOT_PB7_PULSE   = 0xC0  // Generate one pulse on PB7
} ViaTimer1Mode;

typedef enum {
    VIA_SR_DISABLED         = 0x00, // Shift register disabled
    // VIA_SR_IN_T2         = 0x04, // Shift in under T2 control (RESERVED)
    VIA_SR_IN_PHI2          = 0x08, // Shift in under PHI2 control
    VIA_SR_IN_EXT_CB1       = 0x0C, // Shift in under external CB1 clock
    VIA_SR_OUT_FREE_T2      = 0x10, // Shift out free-running at T2 rate (RESERVED)
    VIA_SR_OUT_T2           = 0x14, // Shift out under T2 control (RESERVED)
    VIA_SR_OUT_PHI2         = 0x18, // Shift out under PHI2 control
    VIA_SR_OUT_EXT_CB1      = 0x1C  // Shift out under external CB1 clock
} ViaShiftRegisterMode;

typedef enum {
    VIA_CA1_INT_NEG_EDGE = 0x00, // Interrupt on negative edge
    VIA_CA1_INT_POS_EDGE = 0x01  // Interrupt on positive edge
} ViaCA1InterruptMode; // Note: PCR bit 1 also affects CA1 behavior slightly, but primarily edge select

typedef enum {
    VIA_CB1_INT_NEG_EDGE = 0x00, // Interrupt on negative edge
    VIA_CB1_INT_POS_EDGE = 0x10  // Interrupt on positive edge
} ViaCB1InterruptMode; // Note: PCR bit 5 also affects CB1 behavior slightly, but primarily edge select

typedef enum {
    // Input Modes
    VIA_CA2_INPUT_INT_NEG_EDGE = 0x02, // Input, Interrupt on negative edge
    VIA_CA2_INPUT_INT_POS_EDGE = 0x06, // Input, Interrupt on positive edge
    // Output Modes
    VIA_CA2_OUTPUT_HANDSHAKE   = 0x08, // Handshake output mode
    VIA_CA2_OUTPUT_PULSE       = 0x0A, // Pulse output mode
    VIA_CA2_OUTPUT_MANUAL_LOW  = 0x0C, // Manual output, set low
    VIA_CA2_OUTPUT_MANUAL_HIGH = 0x0E  // Manual output, set high
} ViaCA2Mode;

typedef enum {
    // Input Modes
    VIA_CB2_INPUT_INT_NEG_EDGE = 0x20, // Input, Interrupt on negative edge
    VIA_CB2_INPUT_INT_POS_EDGE = 0x60, // Input, Interrupt on positive edge
    // Output Modes
    VIA_CB2_OUTPUT_HANDSHAKE   = 0x80, // Handshake output mode
    VIA_CB2_OUTPUT_PULSE       = 0xA0, // Pulse output mode
    VIA_CB2_OUTPUT_MANUAL_LOW  = 0xC0, // Manual output, set low
    VIA_CB2_OUTPUT_MANUAL_HIGH = 0xE0  // Manual output, set high
} ViaCB2Mode;

typedef enum {
    VIA_INT_CA1 = VIA_INT_CA1_BIT,
    VIA_INT_CA2 = VIA_INT_CA2_BIT,
    VIA_INT_CB1 = VIA_INT_CB1_BIT,
    VIA_INT_CB2 = VIA_INT_CB2_BIT,
    VIA_INT_SR  = VIA_INT_SR_BIT,
    // VIA_INT_T2 = VIA_INT_T2_BIT, // RESERVED
    VIA_INT_T1  = VIA_INT_T1_BIT,
    VIA_INT_ALL_USER = (VIA_INT_CA1 | VIA_INT_CA2 | VIA_INT_CB1 | VIA_INT_CB2 | VIA_INT_SR | VIA_INT_T1)
} ViaInterruptFlag;


// --- Function Prototypes ---

/**
 * @brief Initialize the VIA to a known safe state.
 * - Ports A & B set to input.
 * - Control lines CA1, CA2, CB1, CB2 set to input/interrupt disabled.
 * - Timer 1 mode set to one-shot interrupt.
 * - Shift register disabled.
 * - Port latches disabled.
 * - All user interrupts disabled and flags cleared.
 * - IMPORTANT: Preserves Timer 2 configuration (ACR bit 5, IER bit 5).
 */
void via_init(void);

// --- Port A Functions ---
void via_set_ddra(uint8_t direction_mask);
uint8_t via_get_ddra(void);
void via_pin_mode_a(uint8_t pin, ViaPinMode mode); // pin 0-7
void via_write_port_a(uint8_t value);
uint8_t via_read_port_a(void); // Reads IRA (handshake, clears CA1/CA2 flags)
uint8_t via_read_port_a_no_handshake(void); // Reads IRA_NHS (no flag clear)
void via_digital_write_a(uint8_t pin, bool value); // pin 0-7
bool via_digital_read_a(uint8_t pin); // pin 0-7 (reads IRA, handshake)

// --- Port B Functions ---
void via_set_ddrb(uint8_t direction_mask);
uint8_t via_get_ddrb(void);
void via_pin_mode_b(uint8_t pin, ViaPinMode mode); // pin 0-7
void via_write_port_b(uint8_t value);
uint8_t via_read_port_b(void);
void via_digital_write_b(uint8_t pin, bool value); // pin 0-7
bool via_digital_read_b(uint8_t pin); // pin 0-7

// --- Timer 1 Functions ---
void via_timer1_set_mode(ViaTimer1Mode mode);
void via_timer1_set_latch(uint16_t value);
void via_timer1_load_and_start(void); // Loads counter from latch, starts countdown
uint16_t via_timer1_read_counter(void); // Clears T1 interrupt flag
uint16_t via_timer1_read_latches(void);

// --- Shift Register Functions ---
void via_shift_register_set_mode(ViaShiftRegisterMode mode);
void via_shift_register_write(uint8_t value);
uint8_t via_shift_register_read(void); // Clears SR interrupt flag

// --- Control Line Functions ---
void via_set_ca1_interrupt_mode(ViaCA1InterruptMode edge_mode);
void via_set_ca2_mode(ViaCA2Mode mode);
void via_set_cb1_interrupt_mode(ViaCB1InterruptMode edge_mode);
void via_set_cb2_mode(ViaCB2Mode mode);
uint8_t via_get_pcr(void); // Get raw PCR value if needed

// --- Latch Functions ---
void via_enable_port_a_latch(void);
void via_disable_port_a_latch(void);
void via_enable_port_b_latch(void);
void via_disable_port_b_latch(void);
uint8_t via_get_acr(void); // Get raw ACR value if needed

// --- Interrupt Functions ---
uint8_t via_get_interrupt_flags(void); // Get raw IFR value
bool via_is_interrupt_pending(ViaInterruptFlag flag); // Check specific flag(s)
void via_clear_interrupt_flags(ViaInterruptFlag flags_mask); // Clear specific flag(s)
void via_enable_interrupts(ViaInterruptFlag flags_mask); // Enable specific interrupt(s)
void via_disable_interrupts(ViaInterruptFlag flags_mask); // Disable specific interrupt(s)
uint8_t via_get_interrupt_enables(void); // Get raw IER value
bool via_is_interrupt_enabled(ViaInterruptFlag flag); // Check if specific interrupt(s) are enabled
bool via_is_irq_active(void); // Check master IRQ status (IFR bit 7)

#endif // VIA6522_H