# Huertuino

Este proyecto consiste en un huerto domótico con Arduino.

Usaremos 
- 1 display LCD 16x2 conectado por bus I2C
- 1sensor BME280 de presión, temperatura y humedad ambiente
- 1 LDR para detectar luz ambiente
- 1 resitencia de 10K
- 1 sensor de humedad de suelo
- 1 sensor de lluvia
- 1 sensor de nivel de agua
- 2 electrobombas
- 2 relés a 5v
- 1 batería de 9v

El funcionamiento, brevemente, es este.

Dado que tenemos dos depósitos, para regar debe haber agua en el principal, estar el terreno seco y no estar lloviendo. Esto es a cualquier hora del día.
Cuando anochece lo detecta el LDR y es el riego diario si tenemos agua y si no llueve.
Si estamos regando y llueve, se para el riego.
El depósito principal si se vacía lo detecta su sensor y nos avisa con sonido y texto en display
Si se va a rebosar, decide regar si el terreno está seco o descargr a un depósito auxiliar.
