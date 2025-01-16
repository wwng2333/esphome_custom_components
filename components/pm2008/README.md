```yaml
# example configuration:

external_components:
  - source: github://wwng2333/esphome_custom_components
    components: [ pm2008 ]

sensor:
  - platform: pm2008
    id: pm25sensor
    pm_2_5:
      name: "PM2.5"
    update_interval: 10s

uart:
  tx_pin: 0
  rx_pin: 1
  baud_rate: 9600
```
