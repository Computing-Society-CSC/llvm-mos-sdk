#include "via.h"

// --- Helper Functions (Internal) ---

// Safely modify ACR, preserving T2 mode bit
static void via_modify_acr(uint8_t clear_mask, uint8_t set_mask) {
    uint8_t current_acr = VIA_REG(VIA_ACR_REG);
    uint8_t preserved_t2_mode = current_acr & VIA_ACR_T2_MODE_BIT;
    uint8_t user_clear_mask = clear_mask & ~VIA_ACR_T2_MODE_BIT; // Don't clear T2 bit
    uint8_t user_set_mask = set_mask & ~VIA_ACR_T2_MODE_BIT;     // Don't set T2 bit
    uint8_t new_acr = (current_acr & ~user_clear_mask) | user_set_mask;
    VIA_REG(VIA_ACR_REG) = new_acr | preserved_t2_mode; // Ensure T2 bit is restored
}

// Safely modify PCR (no T2 bits to preserve here)
static void via_modify_pcr(uint8_t clear_mask, uint8_t set_mask) {
    uint8_t current_pcr = VIA_REG(VIA_PCR_REG);
    uint8_t new_pcr = (current_pcr & ~clear_mask) | set_mask;
    VIA_REG(VIA_PCR_REG) = new_pcr;
}

// Safely enable interrupts in IER, preserving T2 enable bit
static void via_enable_ier_bits(uint8_t enable_mask) {
    uint8_t user_enable_mask = enable_mask & ~VIA_INT_T2_BIT; // Don't enable T2
    if (user_enable_mask) { // Only write if there's something to enable
        VIA_REG(VIA_IER_REG) = VIA_INT_IRQ_BIT | user_enable_mask; // Set enable bits (Bit 7 = 1)
    }
}

// Safely disable interrupts in IER, preserving T2 enable bit
static void via_disable_ier_bits(uint8_t disable_mask) {
    uint8_t user_disable_mask = disable_mask & ~VIA_INT_T2_BIT; // Don't disable T2
    if (user_disable_mask) { // Only write if there's something to disable
        VIA_REG(VIA_IER_REG) = user_disable_mask; // Clear enable bits (Bit 7 = 0)
    }
}

// --- Port A Functions ---

void via_set_ddra(uint8_t direction_mask) {
    VIA_REG(VIA_DDRA_REG) = direction_mask;
}

uint8_t via_get_ddra(void) {
    return VIA_REG(VIA_DDRA_REG);
}

void via_pin_mode_a(uint8_t pin, ViaPinMode mode) {
    if (pin > 7) return;
    uint8_t mask = 1 << pin;
    uint8_t current_ddra = VIA_REG(VIA_DDRA_REG);
    if (mode == VIA_PIN_OUTPUT) {
        VIA_REG(VIA_DDRA_REG) = current_ddra | mask;
    } else {
        VIA_REG(VIA_DDRA_REG) = current_ddra & ~mask;
    }
}

void via_write_port_a(uint8_t value) {
    VIA_REG(VIA_ORA_REG) = value; // Write ORA
}

uint8_t via_read_port_a(void) {
    return VIA_REG(VIA_ORA_REG); // Read IRA (handshake)
}

uint8_t via_read_port_a_no_handshake(void) {
    return VIA_REG(VIA_ORA_NHS_REG); // Read IRA (no handshake)
}

void via_digital_write_a(uint8_t pin, bool value) {
    if (pin > 7) return;
    uint8_t mask = 1 << pin;
    uint8_t current_ora = VIA_REG(VIA_ORA_REG); // Read ORA (doesn't clear flags)
    if (value) {
        VIA_REG(VIA_ORA_REG) = current_ora | mask;
    } else {
        VIA_REG(VIA_ORA_REG) = current_ora & ~mask;
    }
}

bool via_digital_read_a(uint8_t pin) {
    if (pin > 7) return false;
    uint8_t mask = 1 << pin;
    return (VIA_REG(VIA_ORA_REG) & mask) != 0; // Read IRA (handshake)
}

// --- Port B Functions ---

void via_set_ddrb(uint8_t direction_mask) {
    VIA_REG(VIA_DDRB_REG) = direction_mask;
}

uint8_t via_get_ddrb(void) {
    return VIA_REG(VIA_DDRB_REG);
}

void via_pin_mode_b(uint8_t pin, ViaPinMode mode) {
    if (pin > 7) return;
    uint8_t mask = 1 << pin;
    uint8_t current_ddrb = VIA_REG(VIA_DDRB_REG);
    if (mode == VIA_PIN_OUTPUT) {
        VIA_REG(VIA_DDRB_REG) = current_ddrb | mask;
    } else {
        VIA_REG(VIA_DDRB_REG) = current_ddrb & ~mask;
    }
}

void via_write_port_b(uint8_t value) {
    VIA_REG(VIA_ORB_REG) = value; // Write ORB
}

uint8_t via_read_port_b(void) {
    return VIA_REG(VIA_ORB_REG); // Read IRB
}

void via_digital_write_b(uint8_t pin, bool value) {
    if (pin > 7) return;
    uint8_t mask = 1 << pin;
    uint8_t current_orb = VIA_REG(VIA_ORB_REG); // Read ORB (safe)
    if (value) {
        VIA_REG(VIA_ORB_REG) = current_orb | mask;
    } else {
        VIA_REG(VIA_ORB_REG) = current_orb & ~mask;
    }
}

bool via_digital_read_b(uint8_t pin) {
    if (pin > 7) return false;
    uint8_t mask = 1 << pin;
    return (VIA_REG(VIA_ORB_REG) & mask) != 0; // Read IRB
}

// --- Timer 1 Functions ---

void via_timer1_set_mode(ViaTimer1Mode mode) {
    via_modify_acr(VIA_ACR_T1_MODE_MASK, (uint8_t)mode);
}

void via_timer1_set_latch(uint16_t value) {
    VIA_REG(VIA_T1CL_REG) = (uint8_t)(value & 0xFF); // Write low latch byte
    VIA_REG(VIA_T1LH_REG) = (uint8_t)(value >> 8);  // Write high latch byte
}

void via_timer1_load_and_start(void) {
    // Writing high byte of counter loads counter from latch and clears T1 int flag
    VIA_REG(VIA_T1CH_REG) = VIA_REG(VIA_T1LH_REG); // Write high latch value to high counter
}

uint16_t via_timer1_read_counter(void) {
    uint8_t lsb = VIA_REG(VIA_T1CL_REG); // Read low counter byte (latches high byte)
    uint8_t msb = VIA_REG(VIA_T1CH_REG); // Read high counter byte (clears T1 int flag)
    return ((uint16_t)msb << 8) | lsb;
}

uint16_t via_timer1_read_latches(void) {
    uint8_t lsb = VIA_REG(VIA_T1LL_REG); // Read low latch byte
    uint8_t msb = VIA_REG(VIA_T1LH_REG); // Read high latch byte
    return ((uint16_t)msb << 8) | lsb;
}

// --- Shift Register Functions ---

void via_shift_register_set_mode(ViaShiftRegisterMode mode) {
    via_modify_acr(VIA_ACR_SR_MODE_MASK, (uint8_t)mode);
}

void via_shift_register_write(uint8_t value) {
    VIA_REG(VIA_SR_REG) = value;
}

uint8_t via_shift_register_read(void) {
    return VIA_REG(VIA_SR_REG); // Reading clears SR interrupt flag
}

// --- Control Line Functions ---

void via_set_ca1_interrupt_mode(ViaCA1InterruptMode edge_mode) {
    via_modify_pcr(VIA_PCR_CA1_MODE_MASK, (uint8_t)edge_mode);
}

void via_set_ca2_mode(ViaCA2Mode mode) {
    via_modify_pcr(VIA_PCR_CA2_MODE_MASK, (uint8_t)mode);
}

void via_set_cb1_interrupt_mode(ViaCB1InterruptMode edge_mode) {
    via_modify_pcr(VIA_PCR_CB1_MODE_MASK, (uint8_t)edge_mode);
}

void via_set_cb2_mode(ViaCB2Mode mode) {
    via_modify_pcr(VIA_PCR_CB2_MODE_MASK, (uint8_t)mode);
}

uint8_t via_get_pcr(void) {
    return VIA_REG(VIA_PCR_REG);
}

// --- Latch Functions ---

void via_enable_port_a_latch(void) {
    via_modify_acr(0, VIA_ACR_PA_LATCH_EN_BIT);
}

void via_disable_port_a_latch(void) {
    via_modify_acr(VIA_ACR_PA_LATCH_EN_BIT, 0);
}

void via_enable_port_b_latch(void) {
    via_modify_acr(0, VIA_ACR_PB_LATCH_EN_BIT);
}

void via_disable_port_b_latch(void) {
    via_modify_acr(VIA_ACR_PB_LATCH_EN_BIT, 0);
}

uint8_t via_get_acr(void) {
    return VIA_REG(VIA_ACR_REG);
}

// --- Interrupt Functions ---

uint8_t via_get_interrupt_flags(void) {
    return VIA_REG(VIA_IFR_REG);
}

bool via_is_interrupt_pending(ViaInterruptFlag flag) {
    // Check specific flag bit(s) in IFR
    // Note: flag might contain multiple bits if user ORs them
    return (VIA_REG(VIA_IFR_REG) & (uint8_t)flag) != 0;
}

void via_clear_interrupt_flags(ViaInterruptFlag flags_mask) {
    // Write 1s to the specified flag positions to clear them
    // Prevent clearing T2 flag (Bit 5)
    VIA_REG(VIA_IFR_REG) = ((uint8_t)flags_mask & ~VIA_INT_T2_BIT);
}

void via_enable_interrupts(ViaInterruptFlag flags_mask) {
    via_enable_ier_bits((uint8_t)flags_mask);
}

void via_disable_interrupts(ViaInterruptFlag flags_mask) {
    via_disable_ier_bits((uint8_t)flags_mask);
}

uint8_t via_get_interrupt_enables(void) {
    // Read IER. Bit 7 is always read as 1.
    return VIA_REG(VIA_IER_REG);
}

bool via_is_interrupt_enabled(ViaInterruptFlag flag) {
    // Check specific enable bit(s) in IER
    // Note: flag might contain multiple bits if user ORs them
    return (VIA_REG(VIA_IER_REG) & (uint8_t)flag) != 0;
}

bool via_is_irq_active(void) {
    // Check master IRQ status bit (IFR bit 7)
    return (VIA_REG(VIA_IFR_REG) & VIA_INT_IRQ_BIT) != 0;
}