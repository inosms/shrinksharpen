#define cimg_use_jpeg
#include "CImg.h"
#include <iostream>
#include <sstream>
#include <string>

#define INFO(message) std::cout << message << std::endl

using namespace cimg_library;
#define FOREACHCHANNEL(image) for(int channel = 0; channel < image.spectrum(); channel++)

int toInt(std::string n_string)
{
    int tmp_result = 0;
    std::stringstream ss;
    ss << n_string;
    ss >> tmp_result;
    return tmp_result;
}

CImg<float> deriveH(CImg<float>& n_inputImage)
{
    CImg<float> tmp_newDerived(n_inputImage.width(),n_inputImage.height(),1,3,0);

    int x = 0; int y = 0;
    cimg_forY(n_inputImage,y) cimg_forX(n_inputImage,x)
    {
        FOREACHCHANNEL(n_inputImage)
        {
            // the first pixel in a row does not have a previous one (the indices start at 0)
            if( x > 0 )
            {
                tmp_newDerived.atX(x,y,0,channel) = n_inputImage(x,y,0,channel) - n_inputImage(x-1,y,0,channel);
            }
        }
    }
    return tmp_newDerived;
}

void normalizeValues(CImg<float>& n_inputImage)
{
    cimg_forY(n_inputImage,y) cimg_forX(n_inputImage,x)
    {
        FOREACHCHANNEL(n_inputImage)
            n_inputImage(x,y,0,channel) = (n_inputImage(x,y,0,channel)+255)*0.5;
    }
}

CImg<float> applyDerivedH(CImg<float>& n_inputImage, CImg<float>& n_derived)
{
    CImg<float> tmp_newlyCreated(n_inputImage);

    int x = 0; int y = 0;
    cimg_forXY(n_inputImage,x,y)
    {
        FOREACHCHANNEL(n_inputImage)
        {
            if( x > 0 )
            tmp_newlyCreated.atX(x,y,0,channel) = tmp_newlyCreated.atX(x-1,y,0,channel) + n_derived(x,y,0,channel);
        }
    }
    return tmp_newlyCreated;
}

float SHRINK_TRESHOLD = 10.f;
int MIN_SHRINK_SIZE = 2;

float abs(float n_value)
{
    if( n_value < 0 )
        return -n_value;
    return n_value;
}

int signum(float n_value)
{
    if( n_value < 0 )
        return -1;
    else if( n_value == 0 )
        return 0;
    else return 1;
}

// finds for a given start value the end index of this "transition"
// the end is marked as being below the treshold or changing the sign
int findEndH(CImg<float>& n_inputImage,int n_x,int n_y,int n_channel)
{
    int tmp_signumOfStartValue = signum(n_inputImage(n_x,n_y,0,n_channel));

    for( int x = n_x; x < n_inputImage.width(); x++)
    {
        float tmp_derivedValue = n_inputImage(x,n_y,0,n_channel);
        if( abs(tmp_derivedValue) < SHRINK_TRESHOLD || signum(tmp_derivedValue) != tmp_signumOfStartValue )
            return x-1;
    }
    // otherwise it goes to the end
    return n_inputImage.width() - 1;
}

// apply one shrink pass to the image
void shrinkH(CImg<float>& n_inputImage)
{
    int y = 0;
    cimg_forY(n_inputImage,y)
    {
        FOREACHCHANNEL(n_inputImage)
        {
            for(int x = 0; x < n_inputImage.width(); x++ )
            {
                float tmp_derivedValue = n_inputImage(x,y,0,channel);
                if( abs(tmp_derivedValue) >= SHRINK_TRESHOLD )
                {
                    int tmp_endXIndex = findEndH(n_inputImage,x,y,channel);
                    int tmp_diff = tmp_endXIndex - x;

                    // Shrinking means reduzing the size of the transition
                    // therefore the first and the last element of the transition
                    // will be added to even parts on all the elements between
                    // Thus the transition shrinks by 2 pixels each pass

                    // if there are at least 3 elements:
                    // 1 2 3
                    // --> 3 - 1 = 2
                    // minimum default is set to 2 (smaller is not possible)
                    if( tmp_diff >= MIN_SHRINK_SIZE )
                    {
                        // get the pixel values from end and start
                        float tmp_valuesOfStartAndEnd = tmp_derivedValue + n_inputImage(tmp_endXIndex,y,0,channel);

                        // on how many pixels must the values be added?
                        int tmp_parts = tmp_diff - 1;

                        // how much must be added to each pixel between?
                        float tmp_valuePerPart = tmp_valuesOfStartAndEnd/tmp_parts;

                        // reset the first and last element of the transition to 0
                        // -> the transition is now smaller by 2 pixels
                        n_inputImage.atX(tmp_endXIndex,y,0,channel) = 0;
                        n_inputImage.atX(x,y,0,channel) = 0;

                        // finally: add the values from first and last to the pixels
                        // between
                        for( int i = x+1; i < tmp_endXIndex; i++ )
                        {
                            n_inputImage(i,y,0,channel) += tmp_valuePerPart;
                        }

                        // jump to the end of the transition
                        x=tmp_endXIndex;
                    }
                }
            }
        }
    }
}

// will go through the image and make each pixel a whole number
// while adding the rest to the next pixel and so on
// so in the end the image will have no floating values
// (which can not be saved q.q)
void RoundImage(CImg<float>& n_inputImage)
{
    int x = 0; int y = 0;
    FOREACHCHANNEL(n_inputImage) cimg_forY(n_inputImage,y)
    {
        float tmp_fromLast = 0.f;
        cimg_forX(n_inputImage,x)
        {
            float tmp_thisPixelValue = n_inputImage.atX(x,y,0,channel);
            tmp_thisPixelValue += tmp_fromLast;
            float tmp_thisPixelValueRounded = int(tmp_thisPixelValue);
            tmp_fromLast = tmp_thisPixelValue - tmp_thisPixelValueRounded;
            n_inputImage.atX(x,y,0,channel) = tmp_thisPixelValueRounded;

        }
    }
}

void printLine(CImg<float>& n_inputImage,int n_line)
{
    int x = 0;
    FOREACHCHANNEL(n_inputImage)
    {
        std::cout << "channel: " << channel<< ": ";
        cimg_forX(n_inputImage,x)
        {
                std::cout << n_inputImage(x,n_line,0,channel) << " -- ";
        }
        std::cout << std::endl;

    }

}

// clamps the given value to 0,255
float clampColor(float n_input)
{
    if( n_input > 255.f ) return 255.f;
    else if( n_input < 0.f ) return 0.f;
    else return n_input;
}

// clamps each pixel to a value between 0 and 255
// otherwise the image will have visible errors
void ClampImage(CImg<float>& n_inputImage)
{
    int x = 0; int y = 0;
    cimg_forY(n_inputImage,y) cimg_forX(n_inputImage,x) FOREACHCHANNEL(n_inputImage)
    {
        n_inputImage.atX(x,y,0,channel) = clampColor(n_inputImage(x,y,0,channel));
    }
}

int main(int argc, char** argv)
{
    if( argc < 5 )
    {
        INFO("not enough parameters");
        INFO("usage: " << argv[0]<< " input shrink_treshold min_shrink_size passes [output image name]");
        return -1;
    }
    SHRINK_TRESHOLD = toInt(argv[2]);
    MIN_SHRINK_SIZE = toInt(argv[3]);
    int passes = toInt(argv[4]);
    std::string outputFile("output.bmp");
    if( argc > 6 )
        outputFile = std::string(argv[5]);

    if(SHRINK_TRESHOLD < 1 )
    {
        INFO("Shrink treshold must not be below 1, will reset to 1");
        SHRINK_TRESHOLD = 1;
    }
    if( MIN_SHRINK_SIZE < 2 )
    {
        INFO("Min Shrink size below 2 is not possible, will reset to 2");
        MIN_SHRINK_SIZE = 2;
    }
    if( passes < 1 )
    {
        INFO("passes below 2 are not possible, will reset to 1 ");
        passes = 1;
    }

    INFO("Loading image " << argv[1]);
    CImg<float> tmp_inputImage(argv[1]);
    INFO("finished loading");

    // double the size; this will result in better results and
    // no jaggy edges
    tmp_inputImage.resize(-200,-200,-100,-100,5);

    // APPLYING HORZIONTAL STEP
    INFO("starting horizontal shrinking");
    CImg<float>  tmp_derived = deriveH(tmp_inputImage);
    for( int i = 0; i < passes; i++) shrinkH(tmp_derived);
    CImg<float> tmp_afterApply =  ( applyDerivedH(tmp_inputImage,tmp_derived));
    INFO("finished horizontal shrinking");

    // simply rotate the image, and apply another horizontal step
    // (with this I don't have to write a vertical shrink method...)
    tmp_afterApply.rotate(90);

        INFO("starting vertical shrinking");
        tmp_derived = deriveH(tmp_afterApply);
        for( int i = 0; i < passes; i++) shrinkH(tmp_derived);
        // round the derived. Then the resulting image will also have only whole numbers
        // as pixel values
        RoundImage(tmp_derived);
        tmp_afterApply =  ( applyDerivedH(tmp_afterApply,tmp_derived));
        INFO("finished vertical shrinking");

    // rotate back
    tmp_afterApply.rotate(-90);

    // has better results than cubic
    tmp_afterApply.resize_halfXY ();
    // clamping the image makes sure that there are no values above 255 or below 0,
    // otherwise the image would have visible errors
    ClampImage(tmp_afterApply);

    // save the image much wow
    tmp_afterApply.save(outputFile.c_str());

    return 0;
}
