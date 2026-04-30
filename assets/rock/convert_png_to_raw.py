from PIL import Image
import numpy as np

def png_to_raw_rgb(input_path, output_path):
    img = Image.open(input_path)

    if img.mode == "I;16":
        arr = np.array(img, dtype=np.uint16)

        print("16-bit detected")
        print("Min:", arr.min(), "Max:", arr.max())

        arr = (arr - arr.min()) / (arr.max() - arr.min() + 1e-8) * 255
        arr8 = arr.astype(np.uint8)

        rgb = np.stack((arr8, arr8, arr8), axis=-1)

        # Write raw RGB bytes
        with open(output_path, "wb") as f:
            f.write(rgb.tobytes())

    else:
        img = img.convert("RGB")
        pixels = list(img.getdata())

        with open(output_path, "wb") as f:
            for r, g, b in pixels:
                f.write(bytes([r, g, b]))

    print(f"Converted {input_path} -> {output_path}")
    print(f"Resolution: {img.width}x{img.height}")
    print("Format: raw RGB (R,G,B byte sequence)")

# Your batch stays the same
png_to_raw_rgb("rocky_trail_ao_1k.png", "rocky_trail_ao_1k.raw")
png_to_raw_rgb("rocky_trail_arm_1k.png", "rocky_trail_arm_1k.raw")
png_to_raw_rgb("rocky_trail_diff_1k.png", "rocky_trail_diff_1k.raw")
png_to_raw_rgb("rocky_trail_disp_1k.png", "rocky_trail_disp_1k.raw")
png_to_raw_rgb("rocky_trail_nor_dx_1k.png", "rocky_trail_nor_dx_1k.raw")
png_to_raw_rgb("rocky_trail_nor_gl_1k.png", "rocky_trail_nor_gl_1k.raw")
png_to_raw_rgb("rocky_trail_rough_1k.png", "rocky_trail_rough_1k.raw")