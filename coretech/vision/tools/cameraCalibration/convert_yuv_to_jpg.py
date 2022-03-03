import glob
import os
import cv2

# Drop this file in the directory where the images are stored
RAW_IMAGE_DIR = os.path.dirname(os.path.abspath(__file__))
SAVE_IMAGE_DIR = os.path.join(RAW_IMAGE_DIR, "output")
CMD_RUN = "ffmpeg -s 1280x720 -pix_fmt nv21 -i %s -pix_fmt gray16be %s"

if __name__ == "__main__":
        # Get list of raw images
        images_array = []
        for file in glob.glob(os.path.join(RAW_IMAGE_DIR, "*.yuv")):
                images_array.append(file)
        images_array.sort()
        # Create the directory to save the images if needed
        if not os.path.exists(SAVE_IMAGE_DIR):
                print("Creating the directory to save the images")
                os.mkdir(SAVE_IMAGE_DIR)
        # Convert raw images to gray JPEGs
        for image in images_array:
                # os.system("mv " + image + " output")
                image_name = image.split("/")[-1][:-4] + ".jpg"
                file_name = os.path.join(RAW_IMAGE_DIR, image_name)
                # print(file_name)
                os.system(CMD_RUN % (image, file_name))
                cv_img = cv2.imread(file_name, cv2.IMREAD_GRAYSCALE)
                cv2.imwrite(os.path.join(SAVE_IMAGE_DIR, image_name), cv_img)
                os.remove(file_name)
