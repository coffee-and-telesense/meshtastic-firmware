# trunk-ignore-all(ruff/F821)
# trunk-ignore-all(flake8/F821)
import os

Import("env")

freq_band = os.environ.get("MESHTASTIC_FREQ_BAND", "")

if freq_band:
    board_config = env.BoardConfig()
    current_product = board_config.get("build.usb_product", "")
    new_product = current_product + " " + freq_band
    board_config.update("build.usb_product", new_product)
    print(f"USB product name set to: {new_product}")
