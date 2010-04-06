// __BEGIN_LICENSE__
// Copyright (C) 2006-2010 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


// TestImageViewRef.h
#include <gtest/gtest.h>

#include <vw/Image/ImageResource.h>
#include <vw/Image/PixelTypeInfo.h>
#include <vw/Image/PixelTypes.h>

#include <test/Helpers.h>

using namespace vw;

// This tests whether premultiplication preserves integer data.
TEST( ImageResource, PreMultiply ) {
  typedef PixelGrayA<uint16> Px;

  ImageFormat fmt;
  fmt.rows = fmt.cols = 2;
  fmt.planes = 1;
  fmt.pixel_format = PixelFormatID<Px>::value;
  fmt.channel_type = ChannelTypeID<PixelChannelType<Px>::type>::value;

  // Some arbitrary data (48195 triggered a rounding bug)
  // Small alpha AND value will basically always truncate.
  Px buf1_data[4] = {
    Px(10023,255), Px(48195,48195), Px(65535, 3), Px(1,65535)
  };
  Px buf2_data[4], buf3_data[4];

  ImageBuffer buf1(fmt, buf1_data, true);
  ImageBuffer buf2(fmt, buf2_data, false);
  ImageBuffer buf3(fmt, buf3_data, true);

  convert(buf2, buf1, true);
  convert(buf3, buf2, true);

  for (size_t i = 0; i < 4; ++i)
    EXPECT_PIXEL_EQ( buf3_data[i], buf1_data[i] );
}