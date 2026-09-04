#ifndef CODE_IMAGE_H_
#define CODE_IMAGE_H_

#include "zf_common_headfile.h"

#define image_h MT9V03X_H
#define image_w MT9V03X_W
#define TSEL    (127 - image_h + 1)

void image_main(void);

#endif /* CODE_IMAGE_H_ */
