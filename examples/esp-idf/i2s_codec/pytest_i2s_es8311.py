# SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0
import pytest
from pytest_embedded import Dut


@pytest.mark.esp32p4
@pytest.mark.generic
def test_i2s_codec_board_manager(dut: Dut) -> None:
    dut.expect('I \\(([0-9]+)\\) i2s_es8389: I2S codec example start')
    dut.expect('I \\(([0-9]+)\\) i2s_es8389: Board Manager audio devices are ready')
