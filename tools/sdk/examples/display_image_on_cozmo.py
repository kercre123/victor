#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import cozmo
from cozmo.util import degrees
import time
import statistics
import sys
try:
    from PIL import Image
    from PIL import ImageDraw, ImageFont
except ImportError:
    sys.exit("Cannot import from PIL: Do `pip3 install Pillow` to install")

import time


'''Display images on Cozmo's face
'''


def convert_pixel_data_to_bits(pixel_data, image_width, image_height):
    '''
    Args:
        pixel_data (list): list of pixel values, should be in binary (1s or 0s)

    Returns:
        A list of bytes (8 pixels packed per byte)
    '''

    dest_width = 128
    dest_height = 64


    pixel_data = list(zip(*[iter(pixel_data)] * 8)) # convert into 8 pixel chunks - we'll pack each as 1 byte later

    result_list = []

    num_columns_per_pixel = int(dest_width / image_width)
    num_rows_per_pixel = int(dest_height / image_height)

    stride = int(dest_width / 8)

    x = 0
    y = 0
    for l in pixel_data:
        new_byte = 0
        bits_added = 0
        for bit in l:
            for col in range(num_columns_per_pixel):
                new_byte = new_byte << 1
                new_byte += bit
                bits_added += 1
                if (bits_added % 8) == 0:
                    result_list.append(new_byte)
                    x += 1
                    new_byte = 0

        if x == stride:
            x = 0
            y += 1

        if (x==0) and (num_rows_per_pixel > 0):
            # at the end of a row - copy that row for every extra row-per-pixel
            for row in range(num_rows_per_pixel-1):
                start_of_last_row = len(result_list) - stride
                result_list.extend( result_list[start_of_last_row:] )

    return result_list


def convert_image_to_face_data(image, invert_image=False, threshold=200):
    '''
    Args:
        image (Image): A PIL image

    Returns:
        A list of bytes (8 pixels packed per byte)
    '''

    # convert to grayscale, then binary white/black (1/0)
    grayscale_image = image.convert('L')
    if invert_image:
        bw = grayscale_image.point(lambda x: 1 if x < threshold else 0, '1')
    else:
        bw = grayscale_image.point(lambda x: 1 if x > threshold else 0, '1')

    # convert to a flattened one-dimensional list of pixel values (1s or 0s in this case)
    pixel_data = list(bw.getdata())

    # now convert
    return convert_pixel_data_to_bits(pixel_data, image.width, image.height)


def flood_fill_bytes():
    result_list = []

    for i in range(128*8):
        result_list.append(255)

    return result_list


def make_text_image(text_to_print, x, y, font_size):
    '''Make a Pillow.Image with the current time printed on it

    Args:
        width (int): pixel width
        height (int): pixel width

    Returns:
        :class:(Pillow.Image): a Pillow image with the text drawn on it
    '''

    # make a blank image for the text, initialized to opaque black
    width = 128
    height = 32
    txt = Image.new('RGBA', (width, height), (0, 0, 0, 255))

    # get a font - location depends on OS so try a couple of options
    # failing that the default of None will just use a default font

    font = None
    try:
        font = ImageFont.truetype("arial.ttf", font_size)
    except IOError:
        try:
            font = ImageFont.truetype("/Library/Fonts/Arial.ttf", font_size)
        except IOError:
            pass

    # get a drawing context
    dc = ImageDraw.Draw(txt)

    # draw the text
    dc.text((x, y), text_to_print, fill=(255, 255, 255, 255), font=font)

    return txt

def run_fractal_zoomer(coz):

    fractal_width = 128
    fractal_height = 32

    max_fractal_terations = 100  # increase for extra detail, at greater perf cost

    def iterateFractal(inX, inY):
        zr = inX
        zi = inY

        zr2 = zr * zr
        zi2 = zi * zi

        n = max_fractal_terations

        while (((zr2 + zi2) <= 4.0) and (n != 0)):
            n -= 1
            zi = (2.0 * zr * zi) + inY
            zr = zr2 - zi2 + inX

            zr2 = zr * zr
            zi2 = zi * zi

        return n

    xrange_min = 0.00000004470348358154297 # when to stop zooming in

    # for calculating the scaling ratio, use 2* height as we double up the vertical pixels later before
    # displaying on Cozmo's face so the image looks squashed otherwise

    fractal_height2 = fractal_height * 2.0
    xRMult = (fractal_width / fractal_height2) if (fractal_width > fractal_height2) else 1.0
    yRMult = (fractal_height2 / fractal_width) if (fractal_width < fractal_height2) else 1.0

    DEFAULT_XRANGE = 9.6 * xRMult
    DEFAULT_YRANGE = 10.0 * yRMult

    kDefaultMinCX = -4.928734236493008 + (9.6 * 0.5)
    kDefaultMinCY = -4.011056861749877 + (10.0 * 0.5)
    DEFAULT_XMIN = kDefaultMinCX - (DEFAULT_XRANGE * 0.5)
    DEFAULT_YMIN = kDefaultMinCY - (DEFAULT_YRANGE * 0.5)

    mXRange = DEFAULT_XRANGE
    mYRange = DEFAULT_YRANGE
    mXMin = DEFAULT_XMIN
    mYMin = DEFAULT_YMIN

    mDeltaX = mXRange / (fractal_width * 2.0)
    mDeltaY = mYRange / (fractal_height * 2.0)

    def BuildFractal():

        imBytes = []
        dI = 0
        fracY = mYMin + mYRange

        for y in range(fractal_height):

            fracX = mXMin

            for x in range(fractal_width):
                fracVal = iterateFractal(fracX, fracY)
                if fracVal == 0:
                    imBytes.append(0)
                else:
                    imBytes.append( 0 if ((fracVal % 2) == 0) else 1 ) #

                fracX += mDeltaX

            fracY -= mDeltaY
        return imBytes

    while True:
        duration_s = 0.1
        face_data = convert_pixel_data_to_bits( BuildFractal(), fractal_width, fractal_height )
        coz.display_face_image(face_data, duration_s * 1000.0)
        time.sleep(duration_s)
        zoom_scalar = 0.975
        mXMin += (mXRange * (1.0-zoom_scalar) * 0.5);
        mYMin += (mYRange * (1.0-zoom_scalar) * 0.5);
        mXRange = mXRange * zoom_scalar
        mYRange = mYRange * zoom_scalar
        mDeltaX = mXRange / (fractal_width * 2.0)
        mDeltaY = mYRange / (fractal_height * 2.0)


def run_coz_mirror(coz):

    coz.camera.image_stream_enabled = True
    threshold_value = 145
    threshold_value_vel = 0
    while True:
        duration_s = 0.1

        latest_image = coz.world.latest_image

        # try:
        #     latest_image = coz.camera._latest_image
        # except AttributeError as e:
        #     print("failed to get latest_image: %s" % e)
        #     latest_image = None
        if latest_image is not None:

            # print("latest_image = %s" % latest_image)
            resized_image = latest_image.raw_image.resize((128, 32), Image.BICUBIC)

            grayscale_image = resized_image.convert('L')
            pixel_data = list(grayscale_image.getdata())

            total_pixel_val = 0.0
            total_pixel_count = 0
            avg_pixel_value = 0.0
            min_pixel_val =  999
            max_pixel_val = -999
            for pixel in pixel_data:
                total_pixel_val += pixel
                total_pixel_count += 1

                if pixel < min_pixel_val:
                    min_pixel_val = pixel
                if pixel > max_pixel_val:
                    max_pixel_val = pixel

            if total_pixel_count > 0:
                avg_pixel_value = total_pixel_val / total_pixel_count
                try:
                    mode_val = statistics.mode(pixel_data)
                except statistics.StatisticsError:
                    mode_val = 0.0
                print("Average pixel val = avg=%s mode=%s (%s..%s)" % (avg_pixel_value, mode_val, min_pixel_val, max_pixel_val ))

            threshold_value += threshold_value_vel
            if threshold_value > 255:
                threshold_value = 255
                threshold_value_vel = -threshold_value_vel
            elif threshold_value < 0:
                threshold_value = 0
                threshold_value_vel = -threshold_value_vel

            face_data = convert_image_to_face_data( resized_image, threshold=threshold_value )
            #if (len(face_data) > 0):
            coz.display_face_image(face_data, duration_s * 1000.0)
        time.sleep(duration_s)


def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    # move head slightly up. and lift down to the bottom, to make it easy to see Cozmo's face
    coz.set_lift_height(0.0).wait_for_completed()
    coz.set_head_angle(degrees(45.0)).wait_for_completed()

    try:
        run_coz_mirror(coz)
    except KeyboardInterrupt:
        print("bye")
        return

    run_fractal_zoomer(coz)

    anki_logo = Image.open("examples/ankilogo.png")
    anki_logo = anki_logo.resize((128, 32), Image.BICUBIC)
    anki_logo = convert_image_to_face_data(anki_logo, invert_image=True)

    frame_num = 0
    while True:

        frame_num += 1
        if frame_num == 1:
            duration_s = 1.0
            face_data = anki_logo
        elif frame_num == 2:
            duration_s = 1.0
            face_data = convert_image_to_face_data( make_text_image("Cozmo SDK", 10, 8, font_size=20) )
        else:
            duration_s = 1.0
            face_data = convert_image_to_face_data( make_text_image(time.strftime("%I:%M:%S"), 15, 2, font_size=24) )
            frame_num = 0

        coz.display_face_image(face_data, duration_s * 1000.0)
        time.sleep(duration_s)


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
    #cozmo.connect_with_tkviewer(run)

