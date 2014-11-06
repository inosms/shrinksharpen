#define cimg_use_jpeg
#include "CImg.h"
#include <iostream>
#include <sstream>

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

CImg<float> deriveV(CImg<float>& n_inputImage)
{
    CImg<float> tmp_newDerived(n_inputImage.width(),n_inputImage.height(),1,3,0);

    int x = 0; int y = 0;
    cimg_forXY(n_inputImage,x,y)
    {
        FOREACHCHANNEL(n_inputImage)
        {
            if( y > 0 )
            {
                tmp_newDerived.atX(x,y,0,channel) = n_inputImage(x,y,0,channel) - n_inputImage(x,y-1,0,channel);
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

                    // if there are at least 3 elements:
                    // 1 2 3
                    // --> 3 - 1 = 2
                    // minimum default is set to 2 (smaller is not possible)
                    if( tmp_diff >= MIN_SHRINK_SIZE )
                    {
                        float tmp_valuesOfStartAndEnd = tmp_derivedValue + n_inputImage(tmp_endXIndex,y,0,channel);
                        int tmp_parts = tmp_diff - 1;
                        float tmp_valuePerPart = tmp_valuesOfStartAndEnd/tmp_parts;
                        n_inputImage.atX(tmp_endXIndex,y,0,channel) = 0;
                        n_inputImage.atX(x,y,0,channel) = 0;
                        for( int i = x+1; i < tmp_endXIndex; i++ )
                        {
                            n_inputImage(i,y,0,channel) += tmp_valuePerPart;
                        }
                        x=tmp_endXIndex;
                    }
                }
            }
        }
    }
}


void printLine(CImg<float>& n_inputImage,int n_line)
{
    int x = 0;
    cimg_forX(n_inputImage,x)
    {
        std::cout << n_inputImage(x,n_line,0,0) << "\t";
    }
    std::cout << std::endl;
}


int main(int argc, char** argv)
{
    if( argc < 5 )
    {
        INFO("not enough parameters");
        INFO(argv[0]<< " input shrink_treshold min_shrink_size passes");
        return -1;
    }
    SHRINK_TRESHOLD = toInt(argv[2]);
    MIN_SHRINK_SIZE = toInt(argv[3]);
    int passes = toInt(argv[4]);

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
    tmp_inputImage.resize(-200,-200,-100,-100,5);

    // APPLYING HORZIONTAL STEP
    INFO("starting horizontal shrinking");
    CImg<float>  tmp_derived = deriveH(tmp_inputImage);
    for( int i = 0; i < passes; i++) shrinkH(tmp_derived);
    CImg<float> tmp_afterApply =  ( applyDerivedH(tmp_inputImage,tmp_derived));
    INFO("finished horizontal shrinking");

    // simply rotate the image, and apply another horizontal step
    tmp_afterApply.rotate(90);

    INFO("starting vertical shrinking");
    tmp_derived = deriveH(tmp_afterApply);
    for( int i = 0; i < passes; i++) shrinkH(tmp_derived);
    tmp_afterApply =  ( applyDerivedH(tmp_afterApply,tmp_derived));
    INFO("finished vertical shrinking");

    tmp_afterApply.rotate(-90);


    // has better results than cubic
    tmp_afterApply.resize_halfXY ();
    tmp_afterApply.save("output.jpg");

    return 0;
}
