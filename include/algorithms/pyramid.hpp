/*

PICCANTE
The hottest HDR imaging library!
http://vcg.isti.cnr.it/piccante

Copyright (C) 2014
Visual Computing Laboratory - ISTI CNR
http://vcg.isti.cnr.it
First author: Francesco Banterle

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PIC_ALGORITHMS_PYRAMID_HPP
#define PIC_ALGORITHMS_PYRAMID_HPP

#include "../image.hpp"
#include "../image_vec.hpp"
#include "../util/std_util.hpp"
#include "../filtering/filter_gaussian_2d.hpp"
#include "../filtering/filter_sampler_2d.hpp"
#include "../filtering/filter_sampler_2dsub.hpp"
#include "../filtering/filter_sampler_2dadd.hpp"

namespace pic {

/**
 * @brief The Pyramid class
 */
class Pyramid
{
protected:
    bool lapGauss;
    int limitLevel;

    FilterGaussian2D    *flt_gauss;
    FilterSampler2D     *flt_sampler;
    FilterSampler2DSub  *flt_sub;
    FilterSampler2DAdd  *flt_add;

    ImageVec trackerRec, trackerUp;

    /**
     * @brief initFilters
     */
    void initFilters();

    /**
     * @brief create
     * @param img
     * @param width
     * @param height
     * @param channels
     * @param lapGauss
     * @param limitLevel
     */
    void create(Image *img, bool lapGauss, int limitLevel);

public:

    ImageVec stack;

    /**
     * @brief Pyramid
     * @param img
     * @param lapGauss is a boolean parameter. If it is true, a Laplacian pyramid
     * will be created, otherwise a Gaussian one.
     * @param limitLevel
     */
    Pyramid(Image *img, bool lapGauss, int limitLevel);

    /**
     * @brief Pyramid
     * @param width
     * @param height
     * @param channels
     * @param lapGauss is a boolean parameter. If it is true, a Laplacian pyramid
     * will be created, otherwise a Gaussian one.
     * @param limitLevel
     */
    Pyramid(int width, int height, int channels, bool lapGauss, int limitLevel);

    ~Pyramid();

    /**
     * @brief setLapGauss
     * @param lapGauss is a boolean parameter. If it is true, a Laplacian pyramid
     * will be created, otherwise a Gaussian one.
     */
    void setLapGauss(bool lapGauss)
    {
        this->lapGauss = lapGauss;
    }

    /**
     * @brief update recomputes the pyramid given a compatible image, img.
     * @param img
     */
    void update(Image *img);

    /**
     * @brief SetValue
     * @param value
     */
    void setValue(float value);

    /**
     * @brief mul is the mul operator ( *= ) between pyramids.
     * @param pyr
     */
    void mul(const Pyramid *pyr);

    /**
     * @brief add is the add operator ( += ) between pyramids.
     * @param pyr
     */
    void add(const Pyramid *pyr);

    /**
     * @brief reconstruct evaluates a Gaussian/Laplacian pyramid.
     * @param imgOut
     * @return
     */
    Image *reconstruct(Image *imgOut);

    /**
     * @brief blend
     * @param pyr
     * @param weight
     */
    void blend(Pyramid *pyr, Pyramid *weight);

    /**
     * @brief size
     * @return
     */
    int size()
    {
        return int(stack.size());
    }

    /**
     * @brief get
     * @param index
     * @return
     */
    Image *get(int index)
    {
        return stack[index % stack.size()];
    }
};

PIC_INLINE Pyramid::Pyramid(Image *img, bool lapGauss, int limitLevel = 1)
{
    flt_gauss = NULL;
    flt_sampler = NULL;
    flt_sub = NULL;
    flt_add = NULL;

    if(img != NULL) {
        create(img, lapGauss, limitLevel);
    }
}

PIC_INLINE Pyramid::Pyramid(int width, int height, int channels, bool lapGauss, int limitLevel = 1)
{
    flt_gauss = NULL;
    flt_sampler = NULL;
    flt_sub = NULL;
    flt_add = NULL;

    Image *img = new Image(1, width, height, channels);

    create(img, lapGauss, limitLevel);

    delete img;
}

PIC_INLINE Pyramid::~Pyramid()
{
    ImageVecRelease(stack);

    if(flt_gauss != NULL) {
        delete flt_gauss;
        flt_gauss = NULL;
    }

    if(flt_sampler != NULL) {
        delete flt_sampler;
        flt_sampler = NULL;
    }

    if(flt_sub != NULL) {
        delete flt_sub;
        flt_sub = NULL;
    }

    if(flt_add != NULL) {
        delete flt_add;
        flt_add = NULL;
    }
}

PIC_INLINE void Pyramid::initFilters()
{
    if(flt_gauss == NULL) {
        flt_gauss = new FilterGaussian2D(1.0f);
    }

    if(flt_sampler == NULL) {
        flt_sampler = new FilterSampler2D(0.5f);
    }

    if(flt_sub == NULL) {
        flt_sub = new FilterSampler2DSub(NULL);
    }

    if(flt_add == NULL) {
        flt_add = new FilterSampler2DAdd(NULL);
    }
}

PIC_INLINE void Pyramid::create(Image *img, bool lapGauss, int limitLevel = 1)
{
    this->lapGauss  = lapGauss;

    if(limitLevel < 1) {
        limitLevel = 1;
    }

    this->limitLevel = limitLevel;

    initFilters();

    int levels = MAX(log2(MIN(img->width, img->height)) - limitLevel, 1);

    Image *tmpImg = img;

    Image *tmpG = NULL;
    Image *tmpD = NULL;

    for(int i = 0; i < levels; i++) {

        tmpG = flt_gauss->Process(Single(tmpImg), NULL);

        tmpD = flt_sampler->Process(Single(tmpG), NULL);

        if(lapGauss) { //Laplacian Pyramid
            flt_sub->Process(Double(tmpImg, tmpD), tmpG);
            stack.push_back(tmpG);
        } else { //Gaussian Pyramid
            tmpG->assign(tmpImg);
            stack.push_back(tmpG);
        }

        if(i < (levels - 1)) {
            trackerUp.push_back(tmpD);
        }

        tmpImg = tmpD;
    }

    if(tmpD != NULL) {
        stack.push_back(tmpD);
    }

#ifdef PIC_DEBUG
    printf("Pyramid size: %zu\n", stack.size());
#endif
}

PIC_INLINE void Pyramid::update(Image *img)
{
    //TODO: check if the image and the pyramid are compatible
    if(img == NULL) {
        return;
    }

    if(stack.empty() || !img->isValid()) {
        return;
    }

    if(!stack[0]->isSimilarType(img)) {
        return;
    }

    Image *tmpImg = img;

    Image *tmpG = NULL;
    Image *tmpD = NULL;

    unsigned int levels = MAX(log2(MIN(img->width, img->height)) - limitLevel, 1);

    for(unsigned int i = 0; i < levels; i++) {

        tmpG = flt_gauss->Process(Single(tmpImg), stack[i]);

        if(i == (levels - 1)) {
            tmpD = flt_sampler->Process(Single(tmpG), stack[i + 1]);
        } else {
            tmpD = flt_sampler->Process(Single(tmpG), trackerUp[i]);
        }

        if(lapGauss) {	//Laplacian Pyramid
            flt_sub->Process(Double(tmpImg, tmpD), tmpG);
        } else {		//Gaussian Pyramid
            tmpG->assign(tmpImg);
        }

        tmpImg = tmpD;
    }
}

PIC_INLINE Image *Pyramid::reconstruct(Image *imgOut = NULL)
{
    if(stack.size() < 2) {
        return imgOut;
    }

    int n = int(stack.size()) - 1;
    Image *tmp = stack[n];

    if(trackerRec.empty()) {
        for(int i = n; i >= 2; i--) {
            Image *tmp2 = flt_add->Process(Double(stack[i - 1], tmp), NULL);
            trackerRec.push_back(tmp2);
            tmp = tmp2;
        }
    } else {
        int c = 0;

        for(int i = n; i >= 2; i--) {
            flt_add->Process(Double(stack[i - 1], tmp), trackerRec[c]);
            tmp = trackerRec[c];
            c++;
        }
    }    

    imgOut = flt_add->Process(Double(stack[0], tmp), imgOut);

    return imgOut;
}

PIC_INLINE void Pyramid::setValue(float value)
{
    for(unsigned int i = 0; i < stack.size(); i++) {
        *stack[i] = value;
    }
}

PIC_INLINE void Pyramid::mul(const Pyramid *pyr)
{
    if(stack.size() != pyr->stack.size()) {
        return;
    }

    for(unsigned int i = 0; i < stack.size(); i++) {
        *stack[i] *= *pyr->stack[i];
    }
}

PIC_INLINE void Pyramid::add(const Pyramid *pyr)
{
    if(stack.size() != pyr->stack.size()) {
        return;
    }

    for(unsigned int i = 0; i < stack.size(); i++) {
        *stack[i] += *pyr->stack[i];
    }
}

PIC_INLINE void Pyramid::blend(Pyramid *pyr, Pyramid *weight)
{
    if((stack.size() != pyr->stack.size()) && (pyr->stack.size() > 0)) {
        return;
    }

    for(unsigned int i = 0; i < stack.size(); i++) {
        stack[i]->blend(pyr->stack[i], weight->stack[i]);
    }
}

} // end namespace pic

#endif /* PIC_ALGORITHMS_PYRAMID_HPP */

