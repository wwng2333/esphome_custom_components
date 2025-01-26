```yaml
# example configuration:

external_components:
  - source: github://wwng2333/esphome_custom_components
    components: [ cm1108 ]

sensor:
  - platform: empty_uart_sensor
    name: Empty UART sensor

uart:
  tx_pin: D0
  rx_pin: D1
  baud_rate: 9600
```