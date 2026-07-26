`timescale 1ns / 1ps
// CPLD Glue Logic for mc68k SBC - Unicorn v1
// Fixed DTACK + memory map

module glue_logic (
    input wire A1,
    input wire A2,
    input wire A3,
    input wire A4,
    input wire A5,
    input wire A6,
    input wire A7,
    input wire A8,
    input wire A9,
    input wire A10,
    input wire A11,
    input wire A12,
    input wire A13,
    input wire A14,
    input wire A15,
    input wire A16,
    input wire A17,
    input wire A18,
    input wire A19,
    input wire A20,
    input wire A21,
    input wire A22,
    input wire A23,

    input wire AS_n,
    input wire UDS_n,
    input wire LDS_n,
    input wire RW_n,
    input wire CLK,
    input wire RES_n,
    input wire FC2,
    input wire FC1,
    input wire FC0,

    output wire DTACK_n,

    output wire HALT_n,

    output wire ROM_CS_n,
    output wire ROM_LB_OE_n,
    output wire ROM_UB_OE_n,

    output wire RAM_CS_n,
    output wire RAM_LB_OE_n,
    output wire RAM_UB_OE_n,
    output wire RAM_LB_WE_n,
    output wire RAM_UB_WE_n,

    output wire I2C_CS_n,
    output wire I2C_IACK_n,
    input wire I2C_IRQ_n,

    output wire IPL0_n,
    output wire IPL1_n,
    output wire IPL2_n,

    output wire DPUART_CS_n,
    output wire DPUART_IACK_n,
    input wire DPUART_IRQ_n,

    output wire BERR_n
);

    // =============================================================================
    // Memory Map (Word Address)
    // =============================================================================
    localparam [22:0] ROM_END = 23'h00FFFF / 2;
    localparam [22:0] RAM_START = 23'h010000 / 2;
    localparam [22:0] RAM_END = 23'h10FFFF / 2;
    localparam [22:0] DPUART_START = 23'h110000 / 2;
    localparam [22:0] DPUART_END = 23'h1100FF / 2;
    localparam [22:0] I2C_START = 23'h110100 / 2;
    localparam [22:0] I2C_END = 23'h1101FF / 2;

    // Latch address when AS_n is deasserted (address is stable before AS_n goes low)
    reg [22:0] addr_reg;

    reg rom_select;
    reg ram_select;
    reg dpuart_select;
    reg i2c_select;
    reg any_select;

    always @(negedge AS_n) begin        
        addr_reg = {A23,A22,A21,A20,A19,A18,A17,A16,A15,A14,A13,A12,
                     A11,A10,A9 ,A8 ,A7 ,A6 ,A5 ,A4 ,A3 ,A2 ,A1};
        rom_select = (addr_reg <= ROM_END);
        ram_select = (addr_reg >= RAM_START) && (addr_reg <= RAM_END);
        dpuart_select = (addr_reg >= DPUART_START) && (addr_reg <= DPUART_END);
        i2c_select = (addr_reg >= I2C_START) && (addr_reg <= I2C_END);
        any_select = (rom_select | ram_select | dpuart_select | i2c_select);
    end

    wire lower_byte = !LDS_n;
    wire upper_byte = !UDS_n;

    assign RAM_CS_n = !ram_select;
    assign RAM_LB_OE_n = !(ram_select && lower_byte && RW_n);
    assign RAM_UB_OE_n = !(ram_select && upper_byte && RW_n);
    assign RAM_LB_WE_n = !(ram_select && lower_byte && !RW_n);
    assign RAM_UB_WE_n = !(ram_select && upper_byte && !RW_n);

    assign ROM_CS_n = !rom_select;
    assign ROM_LB_OE_n = !(rom_select && lower_byte && RW_n);
    assign ROM_UB_OE_n = !(rom_select && upper_byte && RW_n);

    assign DPUART_CS_n = !(dpuart_select && lower_byte);
    assign I2C_CS_n    = !(i2c_select && lower_byte);

    // Assert DTACK_n one cycle after AS_n has been asserted and a correct address is selected
    reg dtack_n_armed;
    reg dtack_n_value;
    always @(negedge CLK) begin
        if (AS_n == 1) begin
            dtack_n_armed <= 0;
            dtack_n_value <= 1;
        end else if (AS_n == 0 && dtack_n_armed == 0 && any_select) begin
            dtack_n_armed <= 1;
        end else if (AS_n == 0 && dtack_n_armed == 1 && any_select) begin
            dtack_n_value <= 0;
        end
    end

    assign DTACK_n = dtack_n_value;

    // assign DTACK_n = !(!AS_n && any_select);

    assign BERR_n = 1;
    assign HALT_n = RES_n ? 1'bz : 1'b0;

    assign I2C_IACK_n = 1;
    assign DPUART_IACK_n = 1;

    assign IPL0_n = DPUART_IRQ_n;
    assign IPL1_n = (DPUART_IRQ_n && I2C_IRQ_n);
    assign IPL2_n = 1;

endmodule
