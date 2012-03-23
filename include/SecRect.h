/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/*!
 * \file      SecRect.h
 * \brief     header file for SecRect
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2011/06/02
 *
 * <b>Revision History: </b>
 * - 2010/06/03 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 */

/**
 * @page SecRect
 *
 * @section Introduction
 * SetRect is common struct for rectangle
 *
 * @section Copyright
 *  Copyright (c) 2008-2011 Samsung Electronics Co., Ltd.All rights reserved. \n
 *  Proprietary and Confidential
 *
 * @image html samsung.png
 */

#ifndef __SEC_RECT_H__
#define __SEC_RECT_H__

//! Rectangle information
struct SecRect
{
    int x;           //!< x pos
    int y;           //!< y pos
    int w;           //!< width
    int h;           //!< height
    int fullW;       //!< full width of image
    int fullH;       //!< full height of image
    int colorFormat; //!< V4L2_PIX_FMT_XXX

    //! Constructor
    SecRect(int _x_ = 0,
            int _y_ = 0,
            int _w_ = 0,
            int _h_ = 0,
            int _fullW_ = 0,
            int _fullH_ = 0,
            int _colorFormat_ = 0)
    {
        x = _x_;
        y = _y_;
        w = _w_;
        h = _h_;
        fullW = _fullW_;
        fullH = _fullH_;
        colorFormat = _colorFormat_;
    }

    //! Constructor
    SecRect(const SecRect *other)
    {
        x           = other->x;
        y           = other->y;
        w           = other->w;
        h           = other->h;
        fullW       = other->fullW;
        fullH       = other->fullH;
        colorFormat = other->colorFormat;
    }

    //! Operator(=) override
    SecRect& operator =(const SecRect &other)
    {
        x           = other.x;
        y           = other.y;
        w           = other.w;
        h           = other.h;
        fullW       = other.fullW;
        fullH       = other.fullH;
        colorFormat = other.colorFormat;
        return *this;
    }

    //! Operator(==) override
    bool operator ==(const SecRect &other) const
    {
        return (   x           == other.x
                && y           == other.y
                && w           == other.w
                && h           == other.h
                && fullW       == other.fullW
                && fullH       == other.fullH
                && colorFormat == other.colorFormat);
    }

    //! Operator(!=) override
    bool operator !=(const SecRect &other) const
    {
        // use operator(==)
        return !(*this == other);
    }
};

//! Clip information
struct SecRect2
{
    int x1; //!< Left   (The x-coordinate value of upper-left corner)
    int y1; //!< Top    (The y-coordinate value of upper-left corner)
    int x2; //!< Right  (The x-coordinate value of lower-right corner)
    int y2; //!< Bottom (The y-coordinate value of lower-right corner)

    //! Constructor
    SecRect2(int _x1_ = 0, int _y1_ = 0, int _x2_ = 0, int _y2_ = 0)
    {
        x1 = _x1_;
        y1 = _y1_;
        x2 = _x2_;
        y2 = _y2_;
    }

    //! Constructor
    SecRect2(const SecRect2 *other)
    {
        x1 = other->x1;
        y1 = other->y1;
        x2 = other->x2;
        y2 = other->y2;
    }

    //! Operator(=) override
    SecRect2& operator =(const SecRect2 &other)
    {
        x1 = other.x1;
        y1 = other.y1;
        x2 = other.x2;
        y2 = other.y2;
        return *this;
    }

    //! Operator(==) override
    bool operator ==(const SecRect2 &other) const
    {
        return (   x1           == other.x1
                && y1           == other.y1
                && x2           == other.x2
                && y2           == other.y2);
    }

    //! Operator(!=) override
    bool operator !=(const SecRect2 &other) const
    {
        // use operator(==)
        return !(*this == other);
    }
};

#endif //__SEC_RECT_H__
