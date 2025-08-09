from flask import Flask, request, send_file, render_template_string
from flask_cors import CORS
import ctypes
import io
import os


app = Flask(__name__)
CORS(app)


lib_path = os.path.join(os.path.dirname(__file__), 'steganography.so')
stega_lib = ctypes.CDLL(lib_path)


stega_lib.encode.argtypes = [
    ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int,
    ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p)
]
stega_lib.encode.restype = ctypes.c_int

stega_lib.decode.argtypes = [
    ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_char_p
]
stega_lib.decode.restype = None

stega_lib.free_memory.argtypes = [ctypes.c_void_p]
stega_lib.free_memory.restype = None

# --- API Endpoints ---
@app.route('/encode', methods=['POST'])
def encode_image():
    if 'image' not in request.files or 'message' not in request.form:
        return "Error: Missing image or message", 400

    image_file = request.files['image']
    secret_message = request.form['message']

    original_image_bytes = image_file.read()

    file_header = original_image_bytes[:14]
    info_header = original_image_bytes[14:54]

    width = int.from_bytes(info_header[4:8], 'little', signed=True)
    height = abs(int.from_bytes(info_header[8:12], 'little', signed=True))

    pixel_data_offset = int.from_bytes(file_header[10:14], 'little')
    input_pixel_data = original_image_bytes[pixel_data_offset:]

    output_pixel_ptr = ctypes.c_char_p()

    result = stega_lib.encode(
        input_pixel_data, len(input_pixel_data), height, width,
        secret_message.encode('utf-8'),
        ctypes.byref(output_pixel_ptr)
    )

    if result != 0:
        return "Error: The message is too long for this image.", 500

    encoded_pixel_data = ctypes.string_at(output_pixel_ptr, len(input_pixel_data))
    stega_lib.free_memory(output_pixel_ptr)

    final_image_bytes = file_header + info_header + encoded_pixel_data

    return send_file(
        io.BytesIO(final_image_bytes),
        mimetype='image/bmp',
        as_attachment=True,
        download_name='encoded_image.bmp'
    )

@app.route('/decode', methods=['POST'])
def decode_image():
    if 'image' not in request.files:
        return "Error: Missing image", 400

    image_file = request.files['image']
    original_image_bytes = image_file.read()

    info_header = original_image_bytes[14:54]
    width = int.from_bytes(info_header[4:8], 'little', signed=True)
    height = abs(int.from_bytes(info_header[8:12], 'little', signed=True))
    pixel_data_offset = int.from_bytes(original_image_bytes[10:14], 'little')
    pixel_data = original_image_bytes[pixel_data_offset:]

    buffer_size = (width * height * 3) // 8 + 1
    revealed_message_buffer = ctypes.create_string_buffer(buffer_size)

    stega_lib.decode(pixel_data, height, width, revealed_message_buffer)

    return revealed_message_buffer.value.decode('utf-8', errors='ignore')

@app.route('/')
def index():
    return render_template_string(open('index.html').read())

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
