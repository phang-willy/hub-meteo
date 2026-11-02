# Hub météo

Horloge et météo de Paris sur une Freenove ESP32-S3. L'heure vient d'Internet, la météo d'Open-Meteo. La position est fixe : 48,86 °N, 2,35 °E, fuseau de Paris.

Le même programme a deux rendus. En développement, les trois lignes sortent dans le moniteur série du PC. En production, le même texte est dessiné sur l'écran.

```
13°C - Couvert
18:11:12
Mar 27 oct 2026
```

La caméra et le tactile ne sont pas utilisés.

## Technique

| Rôle | Choix |
|---|---|
| Carte | Freenove ESP32-S3-WROOM (FNK0085), programmée comme ESP32S3 Dev Module |
| Langage | Arduino (C++) |
| IDE | Arduino IDE 2, core esp32 d'Espressif |
| Heure | NTP (`pool.ntp.org`, `time.google.com`) |
| Météo | [Open-Meteo](https://api.open-meteo.com/), sans clé |
| Écran | Adafruit GFX et Adafruit ILI9341, bus SPI |
| Console | moniteur série à 115200 bauds |

Le Wi-Fi sert à l'heure et à la météo. Le câble USB alimente la carte et affiche la console. `config.h` est ignoré par git : copier `config.h.exemple` vers `config.h`, puis y mettre le nom et le mot de passe du réseau.

## Matériel

- Freenove ESP32-S3-WROOM, avec ses deux prises USB
- écran IPS 3,2 pouces, 320×240, contrôleur ILI9341, avec tactile
- fils Dupont femelle-femelle

La couleur des fils ne compte pas. Elle sert seulement à les distinguer. Les broches `T_CLK`, `T_CS`, `T_DIN`, `T_DO` et `T_IRQ` restent libres. La nappe caméra aussi.

Avant de brancher `VCC`, lire le marquage de l'écran. S'il indique 3,3 V à 5 V, le fil va sur `5V`. S'il indique seulement `3V3`, ce fil va sur `3V3`. Le fil `LED` reste sur `3V3` dans les deux cas.

## Branchement

Carte débranchée de l'USB. Neuf fils, d'après les noms imprimés sur l'écran et sur la Freenove.

| Écran | Freenove | Rôle |
|---|---|---|
| VCC | 5V | alimentation |
| GND | GND | masse |
| LED | 3V3 | rétroéclairage |
| CS | 14 | sélection de l'écran |
| RESET | 21 | remise à zéro |
| DC (ou RS) | 47 | commande ou pixel |
| SDI (ou MOSI) | 1 | données vers l'écran |
| SCK | 2 | horloge |
| SDO (ou MISO) | 41 | données vers l'ESP32 |

La broche `2` commande aussi la LED de la carte. Elle peut luire pendant le rafraîchissement.

Antenne de la Freenove en haut : `3V3` est en haut à gauche, `5V` en bas à gauche, `GND` en bas à droite. `1`, `2` et `41` sont en haut à droite, `47` et `21` plus bas du même côté.

Après le branchement, passer en mode écran et retéléverser. Le rétroéclairage allumé avec l'heure lisible confirme les fils. Un écran allumé mais vide ou brouillé indique un fil parmi `CS`, `RESET`, `DC`, `SDI` et `SCK` inversé. Un écran noir indique `VCC`, `GND` ou `LED` au mauvais endroit.

## Développement et production

Le choix est la ligne `APP_MODE` de `config.h`.

| | Développement | Production |
|---|---|---|
| Valeur | `MODE_CONSOLE` | `MODE_SCREEN` |
| Où ça s'affiche | moniteur série du PC | écran ILI9341 |
| Écran requis | non | oui, branché comme ci-dessus |
| Code écran | non compilé | compilé |

Les deux modes envoient la même chose : température, heure `HH:MM:SS`, date en français. La météo est relue toutes les 10 minutes. L'heure avance chaque seconde.

Pour passer de l'un à l'autre, modifier `APP_MODE`, puis téléverser à nouveau sur la carte.
