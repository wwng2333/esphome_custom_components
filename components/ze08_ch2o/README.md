```yaml
uart:
  - id: uart_ze08
    rx_pin: GPIO17
    tx_pin: GPIO16
    baud_rate: 9600

external_components:
  - source: github://wwng2333/esphome_custom_components
    components: [ ze08_ch2o ]

ze08_ch2o:
  uart_id: uart_ze08

sensor:
  - platform: ze08_ch2o
    formaldehyde:
      name: "Formaldehyde sensor"
```


Source code from https://github.com/haplm/winsen_ze08_ch2o
