```yaml
# example configuration:

external_components:
  - source: github://wwng2333/esphome_custom_components
    components: [ cm1108 ]

sensor:
  - platform: cm1108
    co2:
      name: CM1108 CO2
    uart_id: uart_1
    warmup_time: 40s
    update_interval: 10s
    automatic_baseline_calibration: false

uart:
  tx_pin: D0
  rx_pin: D1
  baud_rate: 9600
```