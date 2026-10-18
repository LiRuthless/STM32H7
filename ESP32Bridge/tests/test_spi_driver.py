"""Actual production SPI task/pool vs explicit host SDK boundaries."""
import unittest
from spi_host import load


class SpiDriverTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = load()


def scenario(number):
    def run(self):
        result = self.lib.bridge_spi_driver_test(number)
        self.assertEqual(result, 0, 'production SPI scenario failed at C harness line ' + str(result))
    return run


for number, name in enumerate(('driver_pins_mode_affinity_and_dma', 'task_creation_failure', 'semaphore_creation_failure',
                              'wrong_core', 'missing_pool', 'dma_already_owned', 'driver_failure_releases_dma',
                              'duplicate_start_rejected')):
    setattr(SpiDriverTests, 'test_' + name, scenario(number))
