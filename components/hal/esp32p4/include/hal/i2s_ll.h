/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// The LL layer for I2S register operations
/*******************************************************************************
 * NOTICE
 * The hal is not public api, don't use in application code.
 * See readme.md in hal/include/hal/readme.md
 ******************************************************************************/

#pragma once
#include <stdbool.h>
#include "hal/misc.h"
#include "hal/assert.h"
#include "soc/i2s_periph.h"
#include "soc/i2s_struct.h"
#include "soc/soc_etm_struct.h"
#include "soc/hp_sys_clkrst_struct.h"
#include "soc/lp_clkrst_struct.h"
#include "soc/soc_etm_source.h"
#include "hal/i2s_types.h"
#include "hal/hal_utils.h"


#ifdef __cplusplus
extern "C" {
#endif

#define I2S_LL_GET_HW(num)             (((num) == 0)? (&I2S0) : ((num) == 1) ? (&I2S1) : (&I2S2))
#define I2S_LL_GET_ID(hw)              (((hw) == &I2S0)? 0 : ((hw) == &I2S1) ? 1 : 2)

#define I2S_LL_TDM_CH_MASK             (0xffff)
#define I2S_LL_PDM_BCK_FACTOR          (64)

#define I2S_LL_CLK_FRAC_DIV_N_MAX      256 // I2S_MCLK = I2S_SRC_CLK / (N + b/a), the N register is 8 bit-width
#define I2S_LL_CLK_FRAC_DIV_AB_MAX     512 // I2S_MCLK = I2S_SRC_CLK / (N + b/a), the a/b register is 9 bit-width

#define I2S_LL_XTAL_CLK_FREQ           (40 * 1000000)   // XTAL_CLK: 40MHz
#define I2S_LL_DEFAULT_CLK_FREQ        I2S_LL_XTAL_CLK_FREQ  // No PLL clock source on P4, use XTAL as default

#define I2S_LL_ETM_EVENT_TABLE(i2s_port, chan_dir, event)  \
    (uint32_t[SOC_I2S_NUM][2][I2S_ETM_EVENT_MAX]){  \
        [0] = {  \
            [I2S_DIR_RX - 1] = {  \
                [I2S_ETM_EVENT_DONE] = I2S0_EVT_RX_DONE, \
                [I2S_ETM_EVENT_REACH_THRESH] = I2S0_EVT_X_WORDS_RECEIVED,  \
            },  \
            [I2S_DIR_TX - 1] = {  \
                [I2S_ETM_EVENT_DONE] = I2S0_EVT_TX_DONE,  \
                [I2S_ETM_EVENT_REACH_THRESH] = I2S0_EVT_X_WORDS_SENT, \
            },  \
        },  \
        [1] = {  \
            [I2S_DIR_RX - 1] = {  \
                [I2S_ETM_EVENT_DONE] = I2S1_EVT_RX_DONE, \
                [I2S_ETM_EVENT_REACH_THRESH] = I2S1_EVT_X_WORDS_RECEIVED,  \
            },  \
            [I2S_DIR_TX - 1] = {  \
                [I2S_ETM_EVENT_DONE] = I2S1_EVT_TX_DONE,  \
                [I2S_ETM_EVENT_REACH_THRESH] = I2S1_EVT_X_WORDS_SENT, \
            },  \
        },   \
        [2] = {  \
            [I2S_DIR_RX - 1] = {  \
                [I2S_ETM_EVENT_DONE] = I2S2_EVT_RX_DONE, \
                [I2S_ETM_EVENT_REACH_THRESH] = I2S2_EVT_X_WORDS_RECEIVED,  \
            },  \
            [I2S_DIR_TX - 1] = {  \
                [I2S_ETM_EVENT_DONE] = I2S2_EVT_TX_DONE,  \
                [I2S_ETM_EVENT_REACH_THRESH] = I2S2_EVT_X_WORDS_SENT, \
            },  \
        },   \
    }[i2s_port][(chan_dir) - 1][event]


#define I2S_LL_ETM_TASK_TABLE(i2s_port, chan_dir, task)  \
    (uint32_t[SOC_I2S_NUM][2][I2S_ETM_TASK_MAX]){  \
        [0] = {  \
            [I2S_DIR_RX - 1] = {  \
                [I2S_ETM_TASK_START] = I2S0_TASK_START_RX, \
                [I2S_ETM_TASK_STOP] = I2S0_TASK_STOP_RX, \
            },  \
            [I2S_DIR_TX - 1] = {  \
                [I2S_ETM_TASK_START] = I2S0_TASK_START_TX, \
                [I2S_ETM_TASK_STOP] = I2S0_TASK_STOP_TX, \
            },  \
        },  \
        [1] = {  \
            [I2S_DIR_RX - 1] = {  \
                [I2S_ETM_TASK_START] = I2S1_TASK_START_RX, \
                [I2S_ETM_TASK_STOP] = I2S1_TASK_STOP_RX, \
            },  \
            [I2S_DIR_TX - 1] = {  \
                [I2S_ETM_TASK_START] = I2S1_TASK_START_TX, \
                [I2S_ETM_TASK_STOP] = I2S1_TASK_STOP_TX, \
            },  \
        },  \
        [2] = {  \
            [I2S_DIR_RX - 1] = {  \
                [I2S_ETM_TASK_START] = I2S2_TASK_START_RX, \
                [I2S_ETM_TASK_STOP] = I2S2_TASK_STOP_RX, \
            },  \
            [I2S_DIR_TX - 1] = {  \
                [I2S_ETM_TASK_START] = I2S2_TASK_START_TX, \
                [I2S_ETM_TASK_STOP] = I2S2_TASK_STOP_TX, \
            },  \
        },  \
    }[i2s_port][(chan_dir) - 1][task]

#define I2S_LL_ETM_MAX_THRESH_NUM       (0x3FFFUL)

/**
 * @brief Enable the bus clock for I2S module
 *
 * @param i2s_id The port id of I2S
 * @param enable Set true to enable the buf clock
 */
static inline void i2s_ll_enable_bus_clock(int i2s_id, bool enable)
{
    switch (i2s_id) {
        case 0:
            HP_SYS_CLKRST.soc_clk_ctrl2.reg_i2s0_apb_clk_en = enable;
            LP_AON_CLKRST.hp_clk_ctrl.hp_pad_i2s0_mclk_en = enable;
            return;
        case 1:
            HP_SYS_CLKRST.soc_clk_ctrl2.reg_i2s1_apb_clk_en = enable;
            LP_AON_CLKRST.hp_clk_ctrl.hp_pad_i2s1_mclk_en = enable;
            return;
        case 2:
            HP_SYS_CLKRST.soc_clk_ctrl2.reg_i2s2_apb_clk_en = enable;
            LP_AON_CLKRST.hp_clk_ctrl.hp_pad_i2s2_mclk_en = enable;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_enable_bus_clock(...) (void)__DECLARE_RCC_ATOMIC_ENV; i2s_ll_enable_bus_clock(__VA_ARGS__)

/**
 * @brief Reset the I2S module
 *
 * @param i2s_id The port id of I2S
 */
static inline void i2s_ll_reset_register(int i2s_id)
{
    switch (i2s_id) {
        case 0:
            HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_i2s0_apb = 1;
            HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_i2s0_apb = 0;
            return;
        case 1:
            HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_i2s1_apb = 1;
            HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_i2s1_apb = 0;
            return;
        case 2:
            HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_i2s2_apb = 1;
            HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_i2s2_apb = 0;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_reset_register(...) (void)__DECLARE_RCC_ATOMIC_ENV; i2s_ll_reset_register(__VA_ARGS__)

/**
 * @brief I2S module general init, enable I2S clock.
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param enable set true to enable the core clock
 */
static inline void i2s_ll_enable_core_clock(i2s_dev_t *hw, bool enable)
{
    (void)hw;
    (void)enable;
    // No need to do anything
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_enable_core_clock(...) (void)__DECLARE_RCC_ATOMIC_ENV; i2s_ll_enable_core_clock(__VA_ARGS__)

/**
 * @brief Enable I2S tx module clock
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void _i2s_ll_tx_enable_clock(i2s_dev_t *hw)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl13.reg_i2s0_tx_clk_en = 1;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl15.reg_i2s1_tx_clk_en = 1;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl18.reg_i2s2_tx_clk_en = 1;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_tx_enable_clock(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_tx_enable_clock(__VA_ARGS__)

/**
 * @brief Enable I2S rx module clock
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void _i2s_ll_rx_enable_clock(i2s_dev_t *hw)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl11.reg_i2s0_rx_clk_en = 1;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s1_rx_clk_en = 1;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s2_rx_clk_en = 1;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_rx_enable_clock(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_rx_enable_clock(__VA_ARGS__)

/**
 * @brief Disable I2S tx module clock
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_disable_clock(i2s_dev_t *hw)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl13.reg_i2s0_tx_clk_en = 0;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl15.reg_i2s1_tx_clk_en = 0;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl18.reg_i2s2_tx_clk_en = 0;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_tx_disable_clock(...) (void)__DECLARE_RCC_ATOMIC_ENV; i2s_ll_tx_disable_clock(__VA_ARGS__)

/**
 * @brief Disable I2S rx module clock
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_disable_clock(i2s_dev_t *hw)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl11.reg_i2s0_rx_clk_en = 0;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s1_rx_clk_en = 0;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s2_rx_clk_en = 0;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_rx_disable_clock(...) (void)__DECLARE_RCC_ATOMIC_ENV; i2s_ll_rx_disable_clock(__VA_ARGS__)

/**
 * @brief I2S mclk use tx module clock
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void _i2s_ll_mclk_bind_to_tx_clk(i2s_dev_t *hw)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    // Special on P4, set mst_clk_sel to 1 means attach the mclk signal to TX module
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s0_mst_clk_sel = 1;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s1_mst_clk_sel = 1;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl19.reg_i2s2_mst_clk_sel = 1;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_mclk_bind_to_tx_clk(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_mclk_bind_to_tx_clk(__VA_ARGS__)

/**
 * @brief I2S mclk use rx module clock
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void _i2s_ll_mclk_bind_to_rx_clk(i2s_dev_t *hw)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    // Special on P4, set mst_clk_sel to 0 means attach the mclk signal to RX module
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s0_mst_clk_sel = 0;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s1_mst_clk_sel = 0;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl19.reg_i2s2_mst_clk_sel = 0;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_mclk_bind_to_rx_clk(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_mclk_bind_to_rx_clk(__VA_ARGS__)

/**
 * @brief Enable I2S TX slave mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param slave_en Set true to enable slave mode
 */
static inline void i2s_ll_tx_set_slave_mod(i2s_dev_t *hw, bool slave_en)
{
    hw->tx_conf.tx_slave_mod = slave_en;
}

/**
 * @brief Enable I2S RX slave mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param slave_en Set true to enable slave mode
 */
static inline void i2s_ll_rx_set_slave_mod(i2s_dev_t *hw, bool slave_en)
{
    hw->rx_conf.rx_slave_mod = slave_en;
}

/**
 * @brief Reset I2S TX module
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_reset(i2s_dev_t *hw)
{
    hw->tx_conf.tx_reset = 1;
    hw->tx_conf.tx_reset = 0;
}

/**
 * @brief Reset I2S RX module
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_reset(i2s_dev_t *hw)
{
    hw->rx_conf.rx_reset = 1;
    hw->rx_conf.rx_reset = 0;
}

/**
 * @brief Reset I2S TX FIFO
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_reset_fifo(i2s_dev_t *hw)
{
    hw->tx_conf.tx_fifo_reset = 1;
    hw->tx_conf.tx_fifo_reset = 0;
}

/**
 * @brief Reset I2S RX FIFO
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_reset_fifo(i2s_dev_t *hw)
{
    hw->rx_conf.rx_fifo_reset = 1;
    hw->rx_conf.rx_fifo_reset = 0;
}

static inline uint32_t i2s_ll_get_clk_src(i2s_clock_src_t src)
{
    switch (src)
    {
        case I2S_CLK_SRC_XTAL:
            return 0;
        case I2S_CLK_SRC_APLL:
            return 1;
        case I2S_CLK_SRC_EXTERNAL:
            return 2;
        default:
            HAL_ASSERT(false && "unsupported clock source");
            return -1;
    }
}

/**
 * @brief Set TX source clock
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param src I2S source clock.
 */
static inline void _i2s_ll_tx_clk_set_src(i2s_dev_t *hw, i2s_clock_src_t src)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    uint32_t clk_src = i2s_ll_get_clk_src(src);
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl13.reg_i2s0_tx_clk_src_sel = clk_src;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl15.reg_i2s1_tx_clk_src_sel = clk_src;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl18.reg_i2s2_tx_clk_src_sel = clk_src;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_tx_clk_set_src(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_tx_clk_set_src(__VA_ARGS__)

/**
 * @brief Set RX source clock
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param src I2S source clock
 */
static inline void _i2s_ll_rx_clk_set_src(i2s_dev_t *hw, i2s_clock_src_t src)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    uint32_t clk_src = i2s_ll_get_clk_src(src);
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HP_SYS_CLKRST.peri_clk_ctrl11.reg_i2s0_rx_clk_src_sel = clk_src;
            return;
        case 1:
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s1_rx_clk_src_sel = clk_src;
            return;
        case 2:
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s2_rx_clk_src_sel = clk_src;
            return;
    }
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_rx_clk_set_src(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_rx_clk_set_src(__VA_ARGS__)

/**
 * @brief Set I2S tx bck div num
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param val value to set tx bck div num
 */
static inline void i2s_ll_tx_set_bck_div_num(i2s_dev_t *hw, uint32_t val)
{
    hw->tx_conf.tx_bck_div_num = val - 1;
}

/**
 * @brief Set I2S tx raw clock division
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param div_int  Integer part of division
 * @param x  div x
 * @param y  div y
 * @param z  div z
 * @param yn1 yn1
 */
static inline void i2s_ll_tx_set_raw_clk_div(i2s_dev_t *hw, uint32_t div_int, uint32_t x, uint32_t y, uint32_t z, uint32_t yn1)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HAL_FORCE_MODIFY_U32_REG_FIELD(HP_SYS_CLKRST.peri_clk_ctrl13, reg_i2s0_tx_div_n, div_int);
            HP_SYS_CLKRST.peri_clk_ctrl13.reg_i2s0_tx_div_x = x;
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s0_tx_div_y = y;
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s0_tx_div_z = z;
            HP_SYS_CLKRST.peri_clk_ctrl14.reg_i2s0_tx_div_yn1 = yn1;
            return;
        case 1:
            HAL_FORCE_MODIFY_U32_REG_FIELD(HP_SYS_CLKRST.peri_clk_ctrl16, reg_i2s1_tx_div_n, div_int);
            HP_SYS_CLKRST.peri_clk_ctrl16.reg_i2s1_tx_div_x = x;
            HP_SYS_CLKRST.peri_clk_ctrl16.reg_i2s1_tx_div_y = y;
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s1_tx_div_z = z;
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s1_tx_div_yn1 = yn1;
            return;
        case 2:
            HAL_FORCE_MODIFY_U32_REG_FIELD(HP_SYS_CLKRST.peri_clk_ctrl18, reg_i2s2_tx_div_n, div_int);
            HP_SYS_CLKRST.peri_clk_ctrl19.reg_i2s2_tx_div_x = x;
            HP_SYS_CLKRST.peri_clk_ctrl19.reg_i2s2_tx_div_y = y;
            HP_SYS_CLKRST.peri_clk_ctrl19.reg_i2s2_tx_div_z = z;
            HP_SYS_CLKRST.peri_clk_ctrl19.reg_i2s2_tx_div_yn1 = yn1;
            return;
    }
}

/**
 * @brief Set I2S rx raw clock division
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param div_int  Integer part of division
 * @param x  div x
 * @param y  div y
 * @param z  div z
 * @param yn1 yn1
 */
static inline void i2s_ll_rx_set_raw_clk_div(i2s_dev_t *hw, uint32_t div_int, uint32_t x, uint32_t y, uint32_t z, uint32_t yn1)
{
    // Note: this function involves HP_SYS_CLKRST register which is shared with other peripherals, need lock in upper layer
    switch (I2S_LL_GET_ID(hw)) {
        case 0:
            HAL_FORCE_MODIFY_U32_REG_FIELD(HP_SYS_CLKRST.peri_clk_ctrl12, reg_i2s0_rx_div_n, div_int);
            HP_SYS_CLKRST.peri_clk_ctrl12.reg_i2s0_rx_div_x = x;
            HP_SYS_CLKRST.peri_clk_ctrl12.reg_i2s0_rx_div_y = y;
            HP_SYS_CLKRST.peri_clk_ctrl13.reg_i2s0_rx_div_z = z;
            HP_SYS_CLKRST.peri_clk_ctrl13.reg_i2s0_rx_div_yn1 = yn1;
            return;
        case 1:
            HAL_FORCE_MODIFY_U32_REG_FIELD(HP_SYS_CLKRST.peri_clk_ctrl14, reg_i2s1_rx_div_n, div_int);
            HP_SYS_CLKRST.peri_clk_ctrl15.reg_i2s1_rx_div_x = x;
            HP_SYS_CLKRST.peri_clk_ctrl15.reg_i2s1_rx_div_y = y;
            HP_SYS_CLKRST.peri_clk_ctrl15.reg_i2s1_rx_div_z = z;
            HP_SYS_CLKRST.peri_clk_ctrl15.reg_i2s1_rx_div_yn1 = yn1;
            return;
        case 2:
            HAL_FORCE_MODIFY_U32_REG_FIELD(HP_SYS_CLKRST.peri_clk_ctrl17, reg_i2s2_rx_div_n, div_int);
            HP_SYS_CLKRST.peri_clk_ctrl17.reg_i2s2_rx_div_x = x;
            HP_SYS_CLKRST.peri_clk_ctrl18.reg_i2s2_rx_div_y = y;
            HP_SYS_CLKRST.peri_clk_ctrl18.reg_i2s2_rx_div_z = z;
            HP_SYS_CLKRST.peri_clk_ctrl18.reg_i2s2_rx_div_yn1 = yn1;
            return;
    }
}

/**
 * @brief Configure I2S TX module clock divider
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param mclk_div The mclk division coefficients
 */
static inline void _i2s_ll_tx_set_mclk(i2s_dev_t *hw, const hal_utils_clk_div_t *mclk_div)
{
    /* Workaround for inaccurate clock while switching from a relatively low sample rate to a high sample rate
     * Set to particular coefficients first then update to the target coefficients,
     * otherwise the clock division might be inaccurate.
     * the general idea is to set a value that impossible to calculate from the regular decimal */
    i2s_ll_tx_set_raw_clk_div(hw, 7, 317, 7, 3, 0);

    uint32_t div_x = 0;
    uint32_t div_y = 0;
    uint32_t div_z = 0;
    uint32_t div_yn1 = 0;
    /* If any of denominator and numerator is 0, set all the coefficients to 0 */
    if (mclk_div->denominator && mclk_div->numerator) {
        div_yn1 = mclk_div->numerator * 2 > mclk_div->denominator;
        div_z = div_yn1 ? mclk_div->denominator - mclk_div->numerator : mclk_div->numerator;
        div_x = mclk_div->denominator / div_z - 1;
        div_y = mclk_div->denominator % div_z;
    }
    i2s_ll_tx_set_raw_clk_div(hw, mclk_div->integer, div_x, div_y, div_z, div_yn1);
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_tx_set_mclk(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_tx_set_mclk(__VA_ARGS__)

/**
 * @brief Set I2S rx bck div num
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param val value to set rx bck div num
 */
static inline void i2s_ll_rx_set_bck_div_num(i2s_dev_t *hw, uint32_t val)
{
    hw->rx_conf.rx_bck_div_num = val - 1;
}

/**
 * @brief Configure I2S RX module clock divider
 * @note mclk on ESP32 is shared by both TX and RX channel
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param mclk_div The mclk division coefficients
 */
static inline void _i2s_ll_rx_set_mclk(i2s_dev_t *hw, const hal_utils_clk_div_t *mclk_div)
{
    /* Workaround for inaccurate clock while switching from a relatively low sample rate to a high sample rate
     * Set to particular coefficients first then update to the target coefficients,
     * otherwise the clock division might be inaccurate.
     * the general idea is to set a value that impossible to calculate from the regular decimal */
    i2s_ll_rx_set_raw_clk_div(hw, 7, 317, 7, 3, 0);

    uint32_t div_x = 0;
    uint32_t div_y = 0;
    uint32_t div_z = 0;
    uint32_t div_yn1 = 0;
    /* If any of denominator and numerator is 0, set all the coefficients to 0 */
    if (mclk_div->denominator && mclk_div->numerator) {
        div_yn1 = mclk_div->numerator * 2 > mclk_div->denominator;
        div_z = div_yn1 ? mclk_div->denominator - mclk_div->numerator : mclk_div->numerator;
        div_x = mclk_div->denominator / div_z - 1;
        div_y = mclk_div->denominator % div_z;
    }
    i2s_ll_rx_set_raw_clk_div(hw, mclk_div->integer, div_x, div_y, div_z, div_yn1);
}

/// use a macro to wrap the function, force the caller to use it in a critical section
/// the critical section needs to declare the __DECLARE_RCC_ATOMIC_ENV variable in advance
#define i2s_ll_rx_set_mclk(...) (void)__DECLARE_RCC_ATOMIC_ENV; _i2s_ll_rx_set_mclk(__VA_ARGS__)

/**
 * @brief Start I2S TX
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_start(i2s_dev_t *hw)
{
    // Have to update registers before start
    hw->tx_conf.tx_update = 1;
    while (hw->tx_conf.tx_update);
    hw->tx_conf.tx_start = 1;
}

/**
 * @brief Start I2S RX
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_start(i2s_dev_t *hw)
{
    // Have to update registers before start
    hw->rx_conf.rx_update = 1;
    while (hw->rx_conf.rx_update);
    hw->rx_conf.rx_start = 1;
}

/**
 * @brief Stop I2S TX
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_stop(i2s_dev_t *hw)
{
    hw->tx_conf.tx_start = 0;
}

/**
 * @brief Stop I2S RX
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_stop(i2s_dev_t *hw)
{
    hw->rx_conf.rx_start = 0;
}

/**
 * @brief Configure TX WS signal width
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param width WS width in BCK cycle
 */
static inline void i2s_ll_tx_set_ws_width(i2s_dev_t *hw, int width)
{
    hw->tx_conf1.tx_tdm_ws_width = width - 1;
}

/**
 * @brief Configure RX WS signal width
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param width WS width in BCK cycle
 */
static inline void i2s_ll_rx_set_ws_width(i2s_dev_t *hw, int width)
{
    hw->rx_conf1.rx_tdm_ws_width = width - 1;
}

/**
 * @brief Configure the received length to trigger in_suc_eof interrupt
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param eof_num the byte length to trigger in_suc_eof interrupt
 */
static inline void i2s_ll_rx_set_eof_num(i2s_dev_t *hw, int eof_num)
{
    hw->rx_eof_num.rx_eof_num = eof_num;
}

/**
 * @brief Configure TX chan bit and audio data bit
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param chan_bit The chan bit width
 * @param data_bit The audio data bit width
 */
static inline void i2s_ll_tx_set_sample_bit(i2s_dev_t *hw, uint8_t chan_bit, int data_bit)
{
    hw->tx_conf1.tx_bits_mod = data_bit - 1;
    hw->tx_conf1.tx_tdm_chan_bits = chan_bit - 1;
}

/**
 * @brief Configure RX chan bit and audio data bit
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param chan_bit The chan bit width
 * @param data_bit The audio data bit width
 */
static inline void i2s_ll_rx_set_sample_bit(i2s_dev_t *hw, uint8_t chan_bit, int data_bit)
{
    hw->rx_conf1.rx_bits_mod = data_bit - 1;
    hw->rx_conf1.rx_tdm_chan_bits = chan_bit - 1;
}

/**
 * @brief Configure RX half_sample_bit
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param half_sample_bits half sample bit width
 */
static inline void i2s_ll_tx_set_half_sample_bit(i2s_dev_t *hw, int half_sample_bits)
{
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->tx_conf1, tx_half_sample_bits,  half_sample_bits - 1);
}

/**
 * @brief Configure RX half_sample_bit
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param half_sample_bits half sample bit width
 */
static inline void i2s_ll_rx_set_half_sample_bit(i2s_dev_t *hw, int half_sample_bits)
{
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->rx_conf1, rx_half_sample_bits,  half_sample_bits - 1);
}

/**
 * @brief Enable TX MSB shift, the data will be launch at the first BCK clock
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param msb_shift_enable Set true to enable MSB shift
 */
static inline void i2s_ll_tx_enable_msb_shift(i2s_dev_t *hw, bool msb_shift_enable)
{
    hw->tx_conf.tx_msb_shift = msb_shift_enable;
}

/**
 * @brief Enable RX MSB shift, the data will be launch at the first BCK clock
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param msb_shift_enable Set true to enable MSB shift
 */
static inline void i2s_ll_rx_enable_msb_shift(i2s_dev_t *hw, bool msb_shift_enable)
{
    hw->rx_conf.rx_msb_shift = msb_shift_enable;
}

/**
 * @brief Configure TX total chan number
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param total_num Total chan number
 */
static inline void i2s_ll_tx_set_chan_num(i2s_dev_t *hw, int total_num)
{
    hw->tx_tdm_ctrl.tx_tdm_tot_chan_num = total_num - 1;
}

/**
 * @brief Configure RX total chan number
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param total_num Total chan number
 */
static inline void i2s_ll_rx_set_chan_num(i2s_dev_t *hw, int total_num)
{
    hw->rx_tdm_ctrl.rx_tdm_tot_chan_num = total_num - 1;
}

/**
 * @brief Set the bimap of the active TX chan, only the active chan can launch audio data.
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param chan_mask mask of tx active chan
 */
static inline void i2s_ll_tx_set_active_chan_mask(i2s_dev_t *hw, uint32_t chan_mask)
{
    uint32_t tdm_ctrl = hw->tx_tdm_ctrl.val;
    tdm_ctrl &= 0xFFFF0000;
    tdm_ctrl |= chan_mask;
    hw->tx_tdm_ctrl.val = tdm_ctrl;
}

/**
 * @brief Set the bimap of the active RX chan, only the active chan can receive audio data.
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param chan_mask mask of rx active chan
 */
static inline void i2s_ll_rx_set_active_chan_mask(i2s_dev_t *hw, uint32_t chan_mask)
{
    uint32_t tdm_ctrl = hw->rx_tdm_ctrl.val;
    tdm_ctrl &= 0xFFFF0000;
    tdm_ctrl |= chan_mask;
    hw->rx_tdm_ctrl.val = tdm_ctrl;
}

/**
 * @brief Set I2S tx chan mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param slot_mask select slot to send data
 */
static inline void i2s_ll_tx_select_std_slot(i2s_dev_t *hw, i2s_std_slot_mask_t slot_mask)
{
    /* In mono mode, there only should be one slot enabled, another inactive slot will transmit same data as enabled slot
     * Otherwise always enable the first two slots */
    hw->tx_tdm_ctrl.tx_tdm_tot_chan_num = 1;  // tx_tdm_tot_chan_num = 2 slots - 1 = 1
    uint32_t chan_mask = 0;
    switch (slot_mask)
    {
    case I2S_STD_SLOT_LEFT:
        chan_mask |= 0x01;
        break;
    case I2S_STD_SLOT_RIGHT:
        chan_mask |= 0x02;
        break;
    case I2S_STD_SLOT_BOTH:
        chan_mask |= 0x03;
        break;
    default:
        break;
    }
    i2s_ll_tx_set_active_chan_mask(hw, chan_mask);
}

/**
 * @brief Set I2S rx chan mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param slot_mask select slot to receive data
 */
static inline void i2s_ll_rx_select_std_slot(i2s_dev_t *hw, i2s_std_slot_mask_t slot_mask)
{
    /* In mono mode, there only should be one slot enabled, another inactive slot will transmit same data as enabled slot
     * Otherwise always enable the first two slots */
    hw->rx_tdm_ctrl.rx_tdm_tot_chan_num = 1;  // rx_tdm_tot_chan_num = 2 slots - 1 = 1
    uint32_t chan_mask = 0;
    switch (slot_mask)
    {
    case I2S_STD_SLOT_LEFT:
        chan_mask |= 0x01;
        break;
    case I2S_STD_SLOT_RIGHT:
        chan_mask |= 0x02;
        break;
    case I2S_STD_SLOT_BOTH:
        chan_mask |= 0x03;
        break;
    default:
        break;
    }
    i2s_ll_rx_set_active_chan_mask(hw, chan_mask);
}

/**
 * @brief PDM slot mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param mod Channel mode
 *            while tx_ws_idle_pol = 0:
 *            0: stereo
 *            1: Both slots transmit left
 *            2: Both slots transmit right
 *            3: Left transmits `conf_single_data` right transmits data
 *            4: Right transmits `conf_single_data` left transmits data
 *            while tx_ws_idle_pol = 1:
              0: stereo
 *            1: Both slots transmit right
 *            2: Both slots transmit left
 *            3: Right transmits `conf_single_data` left transmits data
 *            4: Left transmits `conf_single_data` right transmits data
 */
static inline void i2s_ll_tx_set_pdm_chan_mod(i2s_dev_t *hw, uint32_t mod)
{
    hw->tx_conf.tx_chan_mod = mod;
}

/**
 * @brief Set TX WS signal pol level
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param ws_pol_level pin level of WS(output) when receiving left channel data
 */
static inline void i2s_ll_tx_set_ws_idle_pol(i2s_dev_t *hw, bool ws_pol_level)
{
    hw->tx_conf.tx_ws_idle_pol = ws_pol_level;
}

/**
 * @brief Set RX WS signal pol level
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param ws_pol_level pin level of WS(input) when receiving left channel data
 */
static inline void i2s_ll_rx_set_ws_idle_pol(i2s_dev_t *hw, bool ws_pol_level)
{
    hw->rx_conf.rx_ws_idle_pol = ws_pol_level;
}

/**
 * @brief Enable I2S TX TDM mode
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_enable_tdm(i2s_dev_t *hw)
{
    hw->tx_conf.tx_pdm_en = false;
    hw->tx_conf.tx_tdm_en = true;
    hw->tx_pcm2pdm_conf.pcm2pdm_conv_en = false;
}

/**
 * @brief Enable I2S RX TDM mode
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_enable_tdm(i2s_dev_t *hw)
{
    hw->rx_conf.rx_pdm_en = false;
    hw->rx_conf.rx_tdm_en = true;
    hw->rx_pdm2pcm_conf.rx_pdm2pcm_en = false;
}

/**
 * @brief Enable I2S TX STD mode
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_enable_std(i2s_dev_t *hw)
{
    i2s_ll_tx_enable_tdm(hw);
}

/**
 * @brief Enable I2S RX STD mode
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_enable_std(i2s_dev_t *hw)
{
    i2s_ll_rx_enable_tdm(hw);
}

/**
 * @brief Enable TX PDM mode.
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_tx_enable_pdm(i2s_dev_t *hw)
{
    hw->tx_conf.tx_pdm_en = true;
    hw->tx_conf.tx_tdm_en = false;
    hw->tx_pcm2pdm_conf.pcm2pdm_conv_en = true;
}

/**
 * @brief Set I2S TX PDM prescale
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param prescale I2S TX PDM prescale
 */
static inline void i2s_ll_tx_set_pdm_prescale(i2s_dev_t *hw, bool prescale)
{
    HAL_FORCE_MODIFY_U32_REG_FIELD(hw->tx_pcm2pdm_conf, tx_pdm_prescale, prescale);
}

/**
 * @brief Set I2S TX PDM high pass filter scaling
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param sig_scale I2S TX PDM signal scaling before transmit to the filter
 */
static inline void i2s_ll_tx_set_pdm_hp_scale(i2s_dev_t *hw, i2s_pdm_sig_scale_t sig_scale)
{
    hw->tx_pcm2pdm_conf.tx_pdm_hp_in_shift = sig_scale;
}

/**
 * @brief Set I2S TX PDM low pass filter scaling
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param sig_scale I2S TX PDM signal scaling before transmit to the filter
 */
static inline void i2s_ll_tx_set_pdm_lp_scale(i2s_dev_t *hw, i2s_pdm_sig_scale_t sig_scale)
{
    hw->tx_pcm2pdm_conf.tx_pdm_lp_in_shift = sig_scale;
}

/**
 * @brief Set I2S TX PDM sinc filter scaling
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param sig_scale I2S TX PDM signal scaling before transmit to the filter
 */
static inline void i2s_ll_tx_set_pdm_sinc_scale(i2s_dev_t *hw, i2s_pdm_sig_scale_t sig_scale)
{
    hw->tx_pcm2pdm_conf.tx_pdm_sinc_in_shift = sig_scale;
}

/**
 * @brief Set I2S TX PDM sigma-delta filter scaling
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param sig_scale I2S TX PDM signal scaling before transmit to the filter
 */
static inline void i2s_ll_tx_set_pdm_sd_scale(i2s_dev_t *hw, i2s_pdm_sig_scale_t sig_scale)
{
    hw->tx_pcm2pdm_conf.tx_pdm_sigmadelta_in_shift = sig_scale;
}

/**
 * @brief Set I2S TX PDM high pass filter param0
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param param The fourth parameter of PDM TX IIR_HP filter stage 1 is (504 + I2S_TX_IIR_HP_MULT12_0[2:0])
 */
static inline void i2s_ll_tx_set_pdm_hp_filter_param0(i2s_dev_t *hw, uint32_t param)
{
    hw->tx_pcm2pdm_conf1.tx_iir_hp_mult12_0 = param;
}

/**
 * @brief Set I2S TX PDM high pass filter param5
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param param The fourth parameter of PDM TX IIR_HP filter stage 2 is (504 + I2S_TX_IIR_HP_MULT12_5[2:0])
 */
static inline void i2s_ll_tx_set_pdm_hp_filter_param5(i2s_dev_t *hw, uint32_t param)
{
    hw->tx_pcm2pdm_conf1.tx_iir_hp_mult12_5 = param;
}

/**
 * @brief Enable I2S TX PDM high pass filter
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param enable Set true to enable I2S TX PDM high pass filter, set false to bypass it
 */
static inline void i2s_ll_tx_enable_pdm_hp_filter(i2s_dev_t *hw, bool enable)
{
    hw->tx_pcm2pdm_conf.tx_pdm_hp_bypass = !enable;
}

/**
 * @brief Set I2S TX PDM sigma-delta codec dither
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param dither I2S TX PDM sigmadelta dither value
 */
static inline void i2s_ll_tx_set_pdm_sd_dither(i2s_dev_t *hw, uint32_t dither)
{
    hw->tx_pcm2pdm_conf.tx_pdm_sigmadelta_dither = dither;
}

/**
 * @brief Set I2S TX PDM sigma-delta codec dither
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param dither2 I2S TX PDM sigmadelta dither2 value
 */
static inline void i2s_ll_tx_set_pdm_sd_dither2(i2s_dev_t *hw, uint32_t dither2)
{
    hw->tx_pcm2pdm_conf.tx_pdm_sigmadelta_dither2 = dither2;
}

/**
 * @brief Set the PDM TX over sampling ratio
 *
 * @param hw  Peripheral I2S hardware instance address.
 * @param ovr Over sampling ratio
 */
static inline void i2s_ll_tx_set_pdm_over_sample_ratio(i2s_dev_t *hw, uint32_t ovr)
{
    hw->tx_pcm2pdm_conf.tx_pdm_sinc_osr2 = ovr;
}

/**
 * @brief Configure I2S TX PDM sample rate
 *        Fpdm = 64*Fpcm*fp/fs
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param fp The fp value of TX PDM filter module group0.
 * @param fs The fs value of TX PDM filter module group0.
 */
static inline void i2s_ll_tx_set_pdm_fpfs(i2s_dev_t *hw, uint32_t fp, uint32_t fs)
{
    hw->tx_pcm2pdm_conf1.tx_pdm_fp = fp;
    hw->tx_pcm2pdm_conf1.tx_pdm_fs = fs;
}

/**
 * @brief Get I2S TX PDM fp configuration parameter
 *
 * @param hw Peripheral I2S hardware instance address.
 * @return
 *        - fp configuration parameter
 */
static inline uint32_t i2s_ll_tx_get_pdm_fp(i2s_dev_t *hw)
{
    return hw->tx_pcm2pdm_conf1.tx_pdm_fp;
}

/**
 * @brief Get I2S TX PDM fs configuration parameter
 *
 * @param hw Peripheral I2S hardware instance address.
 * @return
 *        - fs configuration parameter
 */
static inline uint32_t i2s_ll_tx_get_pdm_fs(i2s_dev_t *hw)
{
    return hw->tx_pcm2pdm_conf1.tx_pdm_fs;
}

/**
 * @brief Enable RX PDM mode.
 *
 * @param hw Peripheral I2S hardware instance address.
 */
static inline void i2s_ll_rx_enable_pdm(i2s_dev_t *hw)
{
    hw->rx_conf.rx_pdm_en = 1;
    hw->rx_conf.rx_tdm_en = 0;
    hw->rx_pdm2pcm_conf.rx_pdm2pcm_en = 1;
}

/**
 * @brief Configure RX PDM downsample
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param dsr PDM downsample configuration parameter
 */
static inline void i2s_ll_rx_set_pdm_dsr(i2s_dev_t *hw, i2s_pdm_dsr_t dsr)
{
    hw->rx_pdm2pcm_conf.rx_pdm_sinc_dsr_16_en = dsr;
}

/**
 * @brief Get RX PDM downsample configuration
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param dsr Pointer to accept PDM downsample configuration
 */
static inline void i2s_ll_rx_get_pdm_dsr(i2s_dev_t *hw, i2s_pdm_dsr_t *dsr)
{
    *dsr = (i2s_pdm_dsr_t)hw->rx_pdm2pcm_conf.rx_pdm_sinc_dsr_16_en;
}

/**
 * @brief Configure RX PDM amplify number
 * @note  This is the amplification number of the digital amplifier,
 *        which is added after the PDM to PCM conversion result and mainly used for
 *        amplify the small PDM signal under the VAD scenario
 *        pcm_result = pdm_input * amplify_num
 *        pcm_result = 0 if amplify_num = 0
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param amp_num PDM RX amplify number
 */
static inline void i2s_ll_rx_set_pdm_amplify_num(i2s_dev_t *hw, uint32_t amp_num)
{
    hw->rx_pdm2pcm_conf.rx_pdm2pcm_amplify_num = amp_num;
}


/**
 * @brief Set I2S RX PDM high pass filter param0
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param param The fourth parameter of PDM RX IIR_HP filter stage 1 is (504 + I2S_RX_IIR_HP_MULT12_0[2:0])
 */
static inline void i2s_ll_rx_set_pdm_hp_filter_param0(i2s_dev_t *hw, uint32_t param)
{
    hw->rx_pdm2pcm_conf.rx_iir_hp_mult12_0 = param;
}

/**
 * @brief Set I2S RX PDM high pass filter param5
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param param The fourth parameter of PDM RX IIR_HP filter stage 2 is (504 + I2S_RX_IIR_HP_MULT12_5[2:0])
 */
static inline void i2s_ll_rx_set_pdm_hp_filter_param5(i2s_dev_t *hw, uint32_t param)
{
    hw->rx_pdm2pcm_conf.rx_iir_hp_mult12_5 = param;
}

/**
 * @brief Enable I2S RX PDM high pass filter
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param enable Set true to enable I2S RX PDM high pass filter, set false to bypass it
 */
static inline void i2s_ll_rx_enable_pdm_hp_filter(i2s_dev_t *hw, bool enable)
{
    hw->rx_pdm2pcm_conf.rx_pdm_hp_bypass = !enable;
}


/**
 * @brief Configura TX a/u-law decompress or compress
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param pcm_cfg PCM configuration parameter
 */
static inline void i2s_ll_tx_set_pcm_type(i2s_dev_t *hw, i2s_pcm_compress_t pcm_cfg)
{
    hw->tx_conf.tx_pcm_conf = pcm_cfg;
    hw->tx_conf.tx_pcm_bypass = !pcm_cfg;
}

/**
 * @brief Configure RX a/u-law decompress or compress
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param pcm_cfg PCM configuration parameter
 */
static inline void i2s_ll_rx_set_pcm_type(i2s_dev_t *hw, i2s_pcm_compress_t pcm_cfg)
{
    hw->rx_conf.rx_pcm_conf = pcm_cfg;
    hw->rx_conf.rx_pcm_bypass = !pcm_cfg;
}

/**
 * @brief Enable TX audio data left alignment
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param ena Set true to enable left alignment
 */
static inline void i2s_ll_tx_enable_left_align(i2s_dev_t *hw, bool ena)
{
    hw->tx_conf.tx_left_align = ena;
}

/**
 * @brief Enable RX audio data left alignment
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param ena Set true to enable left alignment
 */
static inline void i2s_ll_rx_enable_left_align(i2s_dev_t *hw, bool ena)
{
    hw->rx_conf.rx_left_align = ena;
}

/**
 * @brief Enable TX big endian mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param ena Set true to enable big endian mode
 */
static inline void i2s_ll_rx_enable_big_endian(i2s_dev_t *hw, bool ena)
{
    hw->rx_conf.rx_big_endian = ena;
}

/**
 * @brief Enable RX big endian mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param ena Set true to enable big endian mode
 */
static inline void i2s_ll_tx_enable_big_endian(i2s_dev_t *hw, bool ena)
{
    hw->tx_conf.tx_big_endian = ena;
}

/**
 * @brief Configure TX bit order
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param lsb_order_ena Set true to enable LSB bit order
 */
static inline void i2s_ll_tx_set_bit_order(i2s_dev_t *hw, bool lsb_order_ena)
{
    hw->tx_conf.tx_bit_order = lsb_order_ena;
}

/**
 * @brief Configure RX bit order
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param lsb_order_ena Set true to enable LSB bit order
 */
static inline void i2s_ll_rx_set_bit_order(i2s_dev_t *hw, bool lsb_order_ena)
{
    hw->rx_conf.rx_bit_order = lsb_order_ena;
}

/**
 * @brief Configure TX skip mask enable
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param skip_mask_ena Set true to skip inactive channels.
 */
static inline void i2s_ll_tx_set_skip_mask(i2s_dev_t *hw, bool skip_mask_ena)
{
    hw->tx_tdm_ctrl.tx_tdm_skip_msk_en = skip_mask_ena;
}


/**
 * @brief Configure single data
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param data Single data to be set
 */
static inline void i2s_ll_set_single_data(i2s_dev_t *hw, uint32_t data)
{
    hw->conf_single_data.val = data;
}

/**
 * @brief Enable TX mono mode
 * @note MONO in hardware means only one channel got data, but another doesn't
 *       MONO in software means two channel share same data
 *       This function aims to use MONO in software meaning
 *       so 'tx_mono' and 'tx_chan_equal' should be enabled at the same time
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param mono_ena Set true to enable mono mde.
 */
static inline void i2s_ll_tx_enable_mono_mode(i2s_dev_t *hw, bool mono_ena)
{
    hw->tx_conf.tx_mono = mono_ena;
    hw->tx_conf.tx_chan_equal = mono_ena;
}

/**
 * @brief Enable RX mono mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param mono_ena Set true to enable mono mde.
 */
static inline void i2s_ll_rx_enable_mono_mode(i2s_dev_t *hw, bool mono_ena)
{
    hw->rx_conf.rx_mono = mono_ena;
    hw->rx_conf.rx_mono_fst_vld = mono_ena;
}

/**
 * @brief Enable loopback mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param ena Set true to share BCK and WS signal for tx module and rx module.
 */
static inline void i2s_ll_share_bck_ws(i2s_dev_t *hw, bool ena)
{
    hw->tx_conf.sig_loopback = ena;
}

/**
 * @brief PDM TX DMA data take mode
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param is_mono   The DMA data only has one slot (mono) or contains two slots (stereo)
 * @param is_fst_valid  Whether take the DMA data at the first half period
 *                      Only take effet when 'is_mono' is true
 */
static inline void i2s_ll_tx_pdm_dma_take_mode(i2s_dev_t *hw, bool is_mono, bool is_fst_valid)
{
    hw->tx_conf.tx_mono = is_mono;
    hw->tx_conf.tx_mono_fst_vld = is_fst_valid;
}

/**
 * @brief PDM TX slot mode
 * @note     Mode     Left Slot       Right Slot      Chan Mode       WS Pol
 *          -----------------------------------------------------------------
 *           Stereo   Left            Right           0               x
 *          -----------------------------------------------------------------
 *           Mono     Left            Left            1               0
 *           Mono     Right           Right           2               0
 *           Mono     Single          Right           3               0
 *           Mono     Left            Single          4               0
 *          -----------------------------------------------------------------
 *           Mono     Right           Right           1               1
 *           Mono     Left            Left            2               1
 *           Mono     Left            Single          3               1
 *           Mono     Single          Right           4               1
 * @note  The 'Single' above means always sending the value of `conf_single_data` reg
 *        The default value of `conf_single_data` reg is '0', it is not public for now
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param is_mono   The DMA data only has one slot (mono) or contains two slots (stereo)
 * @param is_copy   Whether the un-selected slot copies the data from the selected one
 *                  If not, the un-selected slot will transmit the data from 'conf_single_data'
 * @param mask      The slot mask to select the slot
 */
static inline void i2s_ll_tx_pdm_slot_mode(i2s_dev_t *hw, bool is_mono, bool is_copy, i2s_pdm_slot_mask_t mask)
{
    if (is_mono) {
        /* The default tx_ws_idle_pol is false */
        if (is_copy) {
            hw->tx_conf.tx_chan_mod = mask == I2S_PDM_SLOT_LEFT ? 1 : 2;
        } else {
            hw->tx_conf.tx_chan_mod = mask == I2S_PDM_SLOT_LEFT ? 4 : 3;
        }
    } else {
        hw->tx_conf.tx_chan_mod = 0;
    }
}

/**
 * @brief PDM TX line mode
 * @note    Mode         DAC Mode        2 lines output
 *          -------------------------------------------
 *          PDM codec    0               1
 *          DAC 1-line   1               0
 *          DAC 2-line   1               1
 *
 * @param hw    Peripheral I2S hardware instance address.
 * @param line_mode    PDM TX line mode
 */
static inline void i2s_ll_tx_pdm_line_mode(i2s_dev_t *hw, i2s_pdm_tx_line_mode_t line_mode)
{
    hw->tx_pcm2pdm_conf.tx_pdm_dac_mode_en = line_mode > I2S_PDM_TX_ONE_LINE_CODEC;
    hw->tx_pcm2pdm_conf.tx_pdm_dac_2out_en = line_mode != I2S_PDM_TX_ONE_LINE_DAC;
}

/**
 * @brief  Reset TX FIFO synchronization counter
 *
 * @param hw    Peripheral I2S hardware instance address.
 */
__attribute__((always_inline))
static inline void i2s_ll_tx_reset_fifo_sync_counter(i2s_dev_t *hw)
{
    hw->fifo_cnt.tx_fifo_cnt_rst = 1;
    hw->fifo_cnt.tx_fifo_cnt_rst = 0;
}

/**
 * @brief Get TX FIFO synchronization count value
 *
 * @param hw    Peripheral I2S hardware instance address.
 * @return
 *      bclk count value
 */
__attribute__((always_inline))
static inline uint32_t i2s_ll_tx_get_fifo_sync_count(i2s_dev_t *hw)
{
    return hw->fifo_cnt.tx_fifo_cnt;
}

/**
 * @brief  Reset TX bclk synchronization counter
 *
 * @param hw    Peripheral I2S hardware instance address.
 */
__attribute__((always_inline))
static inline void i2s_ll_tx_reset_bclk_sync_counter(i2s_dev_t *hw)
{
    hw->bck_cnt.tx_bck_cnt_rst = 1;
    hw->bck_cnt.tx_bck_cnt_rst = 0;
}

/**
 * @brief Get TX bclk synchronization count value
 *
 * @param hw    Peripheral I2S hardware instance address.
 * @return
 *      fifo count value
 */
__attribute__((always_inline))
static inline uint32_t i2s_ll_tx_get_bclk_sync_count(i2s_dev_t *hw)
{
    return hw->bck_cnt.tx_bck_cnt;
}

/**
 * @brief Set the TX ETM threshold of REACH_THRESH event
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param thresh The threshold that send
 */
static inline void i2s_ll_tx_set_etm_threshold(i2s_dev_t *hw, uint32_t thresh)
{
    hw->etm_conf.etm_tx_send_word_num = thresh;
}

/**
 * @brief Set the RX ETM threshold of REACH_THRESH event
 *
 * @param hw Peripheral I2S hardware instance address.
 * @param thresh The threshold that received
 */
static inline void i2s_ll_rx_set_etm_threshold(i2s_dev_t *hw, uint32_t thresh)
{
    hw->etm_conf.etm_rx_receive_word_num = thresh;
}

/**
 * @brief Get I2S ETM TX done event status
 *
 * @param hw Peripheral I2S hardware instance address.
 * @return
 *      - true  TX done event triggered
 *      - false TX done event not triggered
 */
static inline bool i2s_ll_get_etm_tx_done_event_status(i2s_dev_t *hw)
{
    uint32_t i2s_id = I2S_LL_GET_ID(hw);
    switch (i2s_id) {
        case 0:
            return SOC_ETM.evt_st4.i2s0_evt_tx_done_st;
        case 1:
            return SOC_ETM.evt_st4.i2s1_evt_tx_done_st;
        case 2:
            return SOC_ETM.evt_st4.i2s2_evt_tx_done_st;
        default:
            HAL_ASSERT(false);
    }
}

/**
 * @brief Get I2S ETM TX done event status
 *
 * @param hw Peripheral I2S hardware instance address.
 * @return
 *      - true  TX done event triggered
 *      - false TX done event not triggered
 */
static inline bool i2s_ll_get_etm_rx_done_event_status(i2s_dev_t *hw)
{
    uint32_t i2s_id = I2S_LL_GET_ID(hw);
    switch (i2s_id) {
        case 0:
            return SOC_ETM.evt_st4.i2s0_evt_rx_done_st;
        case 1:
            return SOC_ETM.evt_st4.i2s1_evt_rx_done_st;
        case 2:
            return SOC_ETM.evt_st4.i2s2_evt_rx_done_st;
        default:
            HAL_ASSERT(false);
    }
}

/**
 * @brief Get I2S ETM TX done event status
 *
 * @param hw Peripheral I2S hardware instance address.
 * @return
 *      - true  TX done event triggered
 *      - false TX done event not triggered
 */
static inline bool i2s_ll_get_etm_tx_threshold_event_status(i2s_dev_t *hw)
{
    uint32_t i2s_id = I2S_LL_GET_ID(hw);
    switch (i2s_id) {
        case 0:
            return SOC_ETM.evt_st4.i2s0_evt_x_words_sent_st;
        case 1:
            return SOC_ETM.evt_st4.i2s1_evt_x_words_sent_st;
        case 2:
            return SOC_ETM.evt_st4.i2s2_evt_x_words_sent_st;
        default:
            HAL_ASSERT(false);
    }
}

/**
 * @brief Get I2S ETM TX done event status
 *
 * @param hw Peripheral I2S hardware instance address.
 * @return
 *      - true  TX done event triggered
 *      - false TX done event not triggered
 */
static inline bool i2s_ll_get_etm_rx_threshold_event_status(i2s_dev_t *hw)
{
    uint32_t i2s_id = I2S_LL_GET_ID(hw);
    switch (i2s_id) {
        case 0:
            return SOC_ETM.evt_st4.i2s0_evt_x_words_received_st;
        case 1:
            return SOC_ETM.evt_st4.i2s1_evt_x_words_received_st;
        case 2:
            return SOC_ETM.evt_st4.i2s2_evt_x_words_received_st;
        default:
            HAL_ASSERT(false);
    }
}

#ifdef __cplusplus
}
#endif
