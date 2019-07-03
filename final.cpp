#include <math.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <vector>


//-----------------------------------------------------------------------------------
// グローバル変数
//-----------------------------------------------------------------------------------
// 視点情報
static double distance = -1.0, pitch = 20.0, yaw = 0.0;
static GLfloat spin = 0.0;

// マウス入力情報
GLint mouse_button = -1;
GLint mouse_x = 0, mouse_y = 0;

//その他のグローバル変数
double WindowID[2];
char* _argv;
int c;
double x_len=0, y_len=0; //入力画像のサイズ
int color = 0; //2Dに表示する画像を切り替えるためのグローバル変数
int nLab;  //等高線の数
double height[100]; //等高線の高さ番号(nLab分入っている)(スケールは10mなど)
double max_h=0; //高さの最大値(高さの正規化のために必要)
int polygon_mode = 0; //ポリゴン生成のモード(0:default, 1:smooth)
double base = 0; //カラーグラデーションのベースライン
double z = 0;


void init (void)
{
  GLfloat light_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
  GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_position[] = { -10.0, -10.0, -10.0, 0.0 };

  glLightfv (GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv (GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv (GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv (GL_LIGHT0, GL_POSITION, light_position);
  glLightfv (GL_LIGHT1, GL_AMBIENT, light_ambient);
  glLightfv (GL_LIGHT1, GL_DIFFUSE, light_diffuse);
  glLightfv (GL_LIGHT1, GL_SPECULAR, light_specular);
  light_position[2] = -light_position[2];
  glLightfv (GL_LIGHT1, GL_POSITION, light_position);

  GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat diffuseMaterial[4] = { 0.5, 0.5, 0.5, 1.0 };
  glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuseMaterial);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
  glMaterialf(GL_FRONT, GL_SHININESS, 25.0);
  glColorMaterial(GL_FRONT, GL_DIFFUSE);
  glEnable(GL_COLOR_MATERIAL);
  
  //glEnable (GL_LIGHTING);
  //glEnable (GL_LIGHT0);
  //glEnable (GL_LIGHT1);   
  glEnable(GL_DEPTH_TEST);
}

void reshape (int w, int h)
{
  glViewport (0, 0, (GLsizei) w, (GLsizei) h); 

  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective(60.0, (GLfloat) w/(GLfloat) h, 0.1, 10.0);

}

//rgb値でグラデーション表現をする関数群
double sigmoid(double x){
  return 1/(1+exp((-20)*(x-0.5)));
}
double colorBar(double x, double *p){
  p[0] = sigmoid(-x+0.9);
  p[2] = sigmoid(x-0.1);
  double yellow;
  if(x<0.5){
    p[1] = sigmoid(x+0.3);
  }else{
    p[1] = sigmoid(-x+1.3);
  }
}
//

//側面ポリゴンを生成する際の二分探索で精度を高めるために必要
int shishagonyu(int a, int b){
  double c = ((double)a)/((double)b);
  double c2 = (double)((int)c);
  if(c>=c2+0.5){
    return (int)(c2+1);
  }else{
    return (int)c2;
  }
}

//cvによる等高線認識
int cv_process (int c, char** a){
  cv::Mat src_img, src_img_gray, tmp_img, dst_img1, dst_img2, dst_img3, dst_img4;
  //ファイルから画像メモリに読み込む
  if (c >= 2)
      src_img = cv::imread(a[1],
                CV_LOAD_IMAGE_COLOR);
  if (!src_img.data)
      return -1;
  //縦が1500pixelになるようにリサイズ
  cv::resize(src_img, src_img, cv::Size(), 1500.0/src_img.cols ,1500.0/src_img.cols);
  //画像の前処理
  cv::cvtColor (src_img, src_img_gray, CV_BGR2GRAY);
  cv::GaussianBlur(src_img_gray, dst_img1, cv::Size(3,3), 500, 500);
  cv::Canny (dst_img1, dst_img2, 50.0, 200.0);
  cv::GaussianBlur(dst_img2, dst_img3, cv::Size(3,3), 100, 100);
  cv::threshold(dst_img3, dst_img4, 70, 255, CV_THRESH_BINARY);
  // ラベル用画像生成(※CV_32S or CV_16Uにする必要あり)
  cv::Mat LabelImg(src_img.size(), CV_32S);
  cv::Mat stats;
  cv::Mat centroids;
  nLab = cv::connectedComponentsWithStats(dst_img4, LabelImg, stats, centroids);
  //等高線高さの取得
  for (int i = 1; i < nLab; ++i) {
    int *param = stats.ptr<int>(i);
    double h=0;  //それぞれの等高線の高さ番号
    int ltx = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];  //ROIの左上のx座標(left top x)
    int lty = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];   //ROIの左上のy座標(left top y)
    int rbx = ltx + param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];  //ROIの右下のx座標(right bottom x)
    int rby = lty + param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];   //ROIの右下のy座標(right bottom y)
    for (int j = 1; j < nLab; ++j) {
      int *param1 = stats.ptr<int>(j);
      int ltx1 = param1[cv::ConnectedComponentsTypes::CC_STAT_LEFT];  //ROIの左上のx座標(left top x)
      int lty1 = param1[cv::ConnectedComponentsTypes::CC_STAT_TOP];   //ROIの左上のy座標(left top y)
      int rbx1 = ltx1 + param1[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];  //ROIの右下のx座標(right bottom x)
      int rby1 = lty1 + param1[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];   //ROIの右下のy座標(right bottom y)
      if(ltx>ltx1 && lty>lty1 && rbx<rbx1 && rby<rby1){ //ROIが他のROIに囲まれているか
	h++; //親のROIの数だけ加算
      }
    }
    height[i] = h*10;
  }
  
  // ラベリング結果の描画色を決定
  for (int i=1; i<nLab; ++i){
    if (height[i]>max_h){
      max_h = height[i];
    }
  }
  std::vector<cv::Vec3b> colors(nLab);
  colors[0] = cv::Vec3b(0, 0, 0);
  for (int i = 1; i < nLab; ++i) {
    double bgr[3] = {0,0,0};
    colorBar(height[i]/max_h,bgr);
    colors[i] = cv::Vec3b((bgr[0]*255), (bgr[1]*255), (bgr[2]*255));
  }
  // ラベリング結果の描画
  cv::Mat Dst(src_img.size(), CV_8UC3);
  for (int i = 0; i < Dst.rows; ++i) {
    int *lb = LabelImg.ptr<int>(i); //ラベル付けした画像のi行の先頭アドレス(各pixelにはラベリング番号が入っている何もないとこるは多分0)
    cv::Vec3b *pix = Dst.ptr<cv::Vec3b>(i); //描写する画像のi行の先頭アドレス
    for (int j = 0; j < Dst.cols; ++j) {
      pix[j] = colors[lb[j]];
    }
  }


  /*
  //ROIの出力
  for (int i = 1; i < nLab; ++i) {
    int *param = stats.ptr<int>(i);
    int x = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];
    int y = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];
    int height = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];
    int width = param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];
    cv::rectangle(Dst, cv::Rect(x, y, width, height), cv::Scalar(0, 255, 0), 2);
  }
  */


  //面積値の出力
  for (int i = 1; i < nLab; ++i) {
    int *param = stats.ptr<int>(i);
    std::cout << "No."<< i << "; height = " << height[i] << "; area = " << param[cv::ConnectedComponentsTypes::CC_STAT_AREA] << std::endl;
    //ROIの左上に高さを書き込む
    int ltx = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];  //ROIの左上のx座標(left top x)
    int lty = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];   //ROIの左上のy座標(left top y)
    int high = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT]; 
    std::stringstream num;
    num << height[i];
    cv::putText(Dst, num.str(), cv::Point(ltx+5, lty+high/2+20), cv::FONT_HERSHEY_COMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
  }

  /*
  //画像の表示
  cv::namedWindow( "Connected Components", CV_WINDOW_AUTOSIZE );
  cv::imshow( "Connected Components", Dst );
  cv::namedWindow ("Pre-processeing4", CV_WINDOW_AUTOSIZE);
  cv::imshow ("Pre-processeing4", dst_img4);
  cv::namedWindow ("Pre-processeing3", CV_WINDOW_AUTOSIZE);
  cv::imshow ("Pre-processeing3", dst_img3);
  cv::namedWindow ("Pre-processeing2", CV_WINDOW_AUTOSIZE);
  cv::imshow ("Pre-processeing2", dst_img2);
  cv::namedWindow ("Pre-processeing1", CV_WINDOW_AUTOSIZE);
  cv::imshow ("Pre-processeing1", dst_img1);
  cv::namedWindow("Image",  CV_WINDOW_AUTOSIZE);
  cv::imshow ("Image", src_img);
  cv::waitKey (0);
  //ウィンドウを消去する
  cv::destroyWindow ("Image");
  cv::destroyWindow ("Pre-processeing1");
  cv::destroyWindow ("Pre-processeing2");
  cv::destroyWindow ("Pre-processeing3");
  cv::destroyWindow ("Pre-processeing4");
  cv::destroyWindow ("Connected Components");
  */

  return 0;
}



void display0(void)
{
  // フレームバッファのクリア
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  //CVによる画像処理
  cv::Mat src_img, src_img_gray, tmp_img, dst_img1, dst_img2, dst_img3, dst_img4;
  src_img = cv::imread(_argv, CV_LOAD_IMAGE_COLOR);
  cv::resize(src_img, src_img, cv::Size(), 1500.0/src_img.cols ,1500.0/src_img.cols);
  cv::cvtColor (src_img, src_img_gray, CV_BGR2GRAY);
  cv::GaussianBlur(src_img_gray, dst_img1, cv::Size(3,3), 500, 500);
  cv::Canny (dst_img1, dst_img2, 50.0, 200.0);
  cv::GaussianBlur(dst_img2, dst_img3, cv::Size(3,3), 100, 100);
  cv::threshold(dst_img3, dst_img4, 70, 255, CV_THRESH_BINARY);
  // ラベル用画像生成(※CV_32S or CV_16Uにする必要あり)
  cv::Mat LabelImg(src_img.size(), CV_32S);
  cv::Mat stats;
  cv::Mat centroids;
  nLab = cv::connectedComponentsWithStats(dst_img4, LabelImg, stats, centroids);
  // ラベリング結果の描画色を決定
  std::vector<cv::Vec3b> colors(nLab);
  colors[0] = cv::Vec3b(0, 0, 0);
  for (int i = 1; i < nLab; ++i) {
    double bgr[3] = {0,0,0};
    colorBar((height[i]+base)/(max_h+base),bgr);
    colors[i] = cv::Vec3b((bgr[0]*255), (bgr[1]*255), (bgr[2]*255));
  }
  // ラベリング結果の描画
  cv::Mat Dst(src_img.size(), CV_8UC3);
  for (int i = 0; i < Dst.rows; ++i) {
    int *lb = LabelImg.ptr<int>(i); //ラベル付けした画像のi行の先頭アドレス(各pixelにはラベリング番号が入っている何もないとこるは多分0)
    cv::Vec3b *pix = Dst.ptr<cv::Vec3b>(i); //描写する画像のi行の先頭アドレス
    for (int j = 0; j < Dst.cols; ++j) {
      pix[j] = colors[lb[j]];
    }
  }
  for (int i = 1; i < nLab; ++i) {
    int *param = stats.ptr<int>(i);
    int ltx = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];  //ROIの左上のx座標(left top x)
    int lty = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];   //ROIの左上のy座標(left top y)
    int high = param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT]; 
    std::stringstream num;
    num << height[i];
    cv::putText(Dst, num.str(), cv::Point(ltx+5, lty+high/2+20), cv::FONT_HERSHEY_COMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
  }

  //CVとGLの間の座礁系変換
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  cv::resize(Dst, Dst, cv::Size(), 0.6,0.6);
  cv::flip(Dst, Dst, 0);
  cv::resize(src_img, src_img, cv::Size(), 0.6,0.6);
  cv::flip(src_img, src_img, 0);
  cv::resize(dst_img4, dst_img4, cv::Size(), 0.6,0.6);
  cv::flip(dst_img4, dst_img4, 0);
  //マウス操作によって、表示する画像を変える
  if(color==1){
    glDrawPixels(Dst.cols ,Dst.rows  , GL_BGR , GL_UNSIGNED_BYTE , Dst.data);
  }else if(color==0){
    glDrawPixels(src_img.cols ,src_img.rows  , GL_BGR , GL_UNSIGNED_BYTE , src_img.data);
  }else if(color==2){
    glDrawPixels(dst_img4.cols ,dst_img4.rows  , GL_LUMINANCE , GL_UNSIGNED_BYTE , dst_img4.data);
  }


  glutSwapBuffers();
}


void display1(void)
{
  // フレームバッファのクリア
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // デプステストを行う
  glEnable( GL_DEPTH_TEST );
  glDepthFunc( GL_LESS );
   // 視点の設定
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0.0, 0.0, distance, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0);  
  glRotatef( -pitch, 1.0, 0.0, 0.0 );
  glRotatef( yaw, 0.0, 1.0, 0.0 );

  //CVによる画像処理
  cv::Mat src_img, src_img_gray, tmp_img, dst_img1, dst_img2, dst_img3, dst_img4;
  src_img = cv::imread(_argv, CV_LOAD_IMAGE_COLOR);
  cv::resize(src_img, src_img, cv::Size(), 1500.0/src_img.cols ,1500.0/src_img.cols);
  cv::cvtColor (src_img, src_img_gray, CV_BGR2GRAY);
  cv::GaussianBlur(src_img_gray, dst_img1, cv::Size(3,3), 500, 500);
  cv::Canny (dst_img1, dst_img2, 50.0, 200.0);
  cv::GaussianBlur(dst_img2, dst_img3, cv::Size(3,3), 100, 100);
  cv::threshold(dst_img3, dst_img4, 70, 255, CV_THRESH_BINARY);
  // ラベル用画像生成(※CV_32S or CV_16Uにする必要あり)
  cv::Mat LabelImg(src_img.size(), CV_32S);
  cv::Mat stats;
  cv::Mat centroids;
  int nLab = cv::connectedComponentsWithStats(dst_img4, LabelImg, stats, centroids);
  // ラベリング結果の描画色を決定
  std::vector<cv::Vec3b> colors(nLab);
  colors[0] = cv::Vec3b(0, 0, 0);
  for (int i = 1; i < nLab; ++i) {
    double bgr[3] = {0,0,0};
    colorBar(height[i]/max_h,bgr);
    colors[i] = cv::Vec3b((bgr[0]*255), (bgr[1]*255), (bgr[2]*255));
  }
  // 等高面のポリゴン生成
  int line_len[nLab]; //各等高線上の点の数を記憶しておく
  for (int k = 1; k < nLab; ++k) {
    cv::Mat Dst(src_img.size(), CV_8UC3);
    for (int i = 0; i < Dst.rows; ++i) {
      int *lb = LabelImg.ptr<int>(i); //ラベル付けした画像のi行の先頭アドレス(各pixelにはラベリング番号が入っている何もないとこるは多分0)
      cv::Vec3b *pix = Dst.ptr<cv::Vec3b>(i); //描写する画像のi行の先頭アドレス
      for (int j = 0; j < Dst.cols; ++j) {
	if(lb[j]==k){
	  pix[j] = colors[lb[j]];
	}else{
	  pix[j] = colors[0];
	}
      }
    }
    cv::cvtColor (Dst, Dst, CV_BGR2GRAY);
    cv::threshold(Dst, Dst, 10, 255, CV_THRESH_BINARY);
    int tmp = 0;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(Dst, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);
    double bgr[3] = {0,0,0};
    colorBar((height[k]+base)/(max_h+base),bgr);
    for(std::vector<std::vector<cv::Point>>::iterator it1 = contours.begin(); it1 != contours.end(); it1++){
      glColor3f (bgr[2],bgr[1],bgr[0]);
      glBegin(GL_POLYGON);
      for(std::vector<cv::Point>::iterator it2 = it1->begin(); it2 != it1->end(); it2++){
	cv::Point p = *it2;
	tmp++;
	glVertex3f(p.x/1500.0-0.5, p.y/1500.0-0.5, -height[k]/250.0);
      }
      glEnd();
    }
    line_len[k] = tmp;
  }

  
  //側面のポリゴン生成
  for (int k = nLab-1; k > 1; --k) {
    cv::Mat Dst1(src_img.size(), CV_8UC3);
    for (int i = 0; i < Dst1.rows; ++i) {
      int *lb = LabelImg.ptr<int>(i); //ラベル付けした画像のi行の先頭アドレス(各pixelにはラベリング番号が入っている何もないとこるは多分0)
      cv::Vec3b *pix = Dst1.ptr<cv::Vec3b>(i); //描写する画像のi行の先頭アドレス
      for (int j = 0; j < Dst1.cols; ++j) {
	if(lb[j]==k){
	  pix[j] = colors[lb[j]];
	}else{
	  pix[j] = colors[0];
	}
      }
    }
    cv::cvtColor (Dst1, Dst1, CV_BGR2GRAY);
    cv::threshold(Dst1, Dst1, 10, 255, CV_THRESH_BINARY);


    //今の等高線kのすぐ外側の等高線番号を取得する
    int *param = stats.ptr<int>(k);
    int k2=0;  //ほしい等高線番号
    int ltx = param[cv::ConnectedComponentsTypes::CC_STAT_LEFT];  //ROIの左上のx座標(left top x)
    int lty = param[cv::ConnectedComponentsTypes::CC_STAT_TOP];   //ROIの左上のy座標(left top y)
    int rbx = ltx + param[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];  //ROIの右下のx座標(right bottom x)
    int rby = lty + param[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];   //ROIの右下のy座標(right bottom y)
    for (int j = 1; j < nLab; ++j) {
      int *param1 = stats.ptr<int>(j);
      int ltx1 = param1[cv::ConnectedComponentsTypes::CC_STAT_LEFT];  //ROIの左上のx座標(left top x)
      int lty1 = param1[cv::ConnectedComponentsTypes::CC_STAT_TOP];   //ROIの左上のy座標(left top y)
      int rbx1 = ltx1 + param1[cv::ConnectedComponentsTypes::CC_STAT_WIDTH];  //ROIの右下のx座標(right bottom x)
      int rby1 = lty1 + param1[cv::ConnectedComponentsTypes::CC_STAT_HEIGHT];   //ROIの右下のy座標(right bottom y)
      if(ltx>ltx1 && lty>lty1 && rbx<rbx1 && rby<rby1 && height[j]+10==height[k]){  //今のROIが
	k2 = j;
      }
    }
    cv::Mat Dst2(src_img.size(), CV_8UC3);
    for (int i = 0; i < Dst2.rows; ++i) {
      int *lb = LabelImg.ptr<int>(i); //ラベル付けした画像のi行の先頭アドレス(各pixelにはラベリング番号が入っている何もないとこるは多分0)
      cv::Vec3b *pix = Dst2.ptr<cv::Vec3b>(i); //描写する画像のi行の先頭アドレス
      for (int j = 0; j < Dst2.cols; ++j) {
	if(lb[j]==k2){
	  pix[j] = colors[lb[j]];
	}else{
	  pix[j] = colors[0];
	}
      }
    }
    cv::cvtColor (Dst2, Dst2, CV_BGR2GRAY);
    cv::threshold(Dst2, Dst2, 10, 255, CV_THRESH_BINARY);

    
    std::vector<std::vector<cv::Point>> contours1;
    std::vector<cv::Vec4i> hierarchy1;
    cv::findContours(Dst1, contours1, hierarchy1, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);
    std::vector<std::vector<cv::Point>> contours2;
    std::vector<cv::Vec4i> hierarchy2;
    cv::findContours(Dst2, contours2, hierarchy2, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_TC89_L1);

    double bgr[3] = {0,0,0};
    colorBar((height[k]+base)/(max_h+base),bgr);
    double bgr2[3] = {0,0,0};
    colorBar((height[k2]+base)/(max_h+base),bgr2);


    if(polygon_mode==0){
      //側面ポリゴン作成の際に下の等高線上の点のステップ間隔を決める(間隔の数はline_len[k]-1)
      int loss = line_len[k2]-line_len[k];
      int steps[line_len[k]];
      for(int i=0; i<line_len[k]; i++){
	steps[i] = loss/line_len[k];
      }

      int t = loss%line_len[k]; //stepsの全てのインデックスに均等に配布したあとの余り(これを二分探索の要領で散らす)
      int ln[10000]; //stepsのうちに残り数tを置くインデックスを記憶
      for(int i=0; i<10000; i++){
	ln[i]=0;
      }
      int tmp_ln[10000];
      for(int i=0; i<10000; i++){
	tmp_ln[i]=0;
      }
      ln[0]=0; ln[1]=line_len[k];
      tmp_ln[0]=shishagonyu(line_len[k],2);
    
      int count = 1;
      for(int i=0; i<200; i++){
	if(((int)pow(2.0,(double)count)+1)>t){     //count=n+1の総数P(n+1)が余りを超えたら終了（count=n, len(ln|count)=2**(n-1)+1）
	  break;
	}
	for(int j=0; j<(int)pow(2.0,(double)(count-1)); j++){
	  tmp_ln[j]=shishagonyu(ln[j]+ln[j+1],2);
	}
	for(int k=(int)pow(2.0,(double)(count-1)); k>=0; k--){
	  int tmp=ln[k];
	  ln[k*2]=tmp;
	}
	for(int k=0; k<(int)pow(2.0,(double)(count-1)); k++){
	  ln[2*k+1]=tmp_ln[k];
	}
	count++;
      }

      int y = (int)pow(2.0,(double)(count-1))+1;
      for(int i=0; i<y; i++){
	steps[ln[i]]+=1;
      }
      for(int i=0; i<(t-y); i++){
	steps[shishagonyu(ln[i]+ln[i+1],2)]+=1;
      }
      
      std::vector<std::vector<cv::Point>>::iterator it12 = contours2.begin();
      std::vector<cv::Point>::iterator it22 = it12->begin();
      int j=0;
      for(std::vector<std::vector<cv::Point>>::iterator it1 = contours1.begin(); it1 != contours1.end(); it1++){
	glBegin(GL_TRIANGLE_STRIP);
	for(std::vector<cv::Point>::iterator it2 = it1->begin(); it2 != it1->end(); it2++){
	  cv::Point p = *it2;
	  cv::Point p2 = *it22;
	  glColor3f (bgr[2],bgr[1],bgr[0]);
	  glVertex3f(p.x/1500.0-0.5, p.y/1500.0-0.5, -height[k]/250.0);
	  for(int i=0; i<steps[j]; i++){
	    it22++;
	  }
	  glColor3f (bgr2[2],bgr2[1],bgr2[0]);
	  glVertex3f(p2.x/1500.0-0.5, p2.y/1500.0-0.5, -height[k2]/250.0);
	  it22++;
	  j++;
	}
      
	//最後に終端と始点を結ぶ
	std::vector<std::vector<cv::Point>>::iterator it3 = contours1.begin();
	std::vector<cv::Point>::iterator it4 = it1->begin();
	std::vector<std::vector<cv::Point>>::iterator it32 = contours2.begin();
	std::vector<cv::Point>::iterator it42 = it12->begin();
	cv::Point p = *it4;
	cv::Point p2 = *it42;
	glColor3f (bgr[2],bgr[1],bgr[0]);
	glVertex3f(p.x/1500.0-0.5, p.y/1500.0-0.5, -height[k]/250.0);
	glColor3f (bgr2[2],bgr2[1],bgr2[0]);
	glVertex3f(p2.x/1500.0-0.5, p2.y/1500.0-0.5, -height[k2]/250.0);
	//
      
	glEnd();
      }
    }else{
      std::vector<std::vector<cv::Point>>::iterator it12 = contours2.begin();
      for(std::vector<std::vector<cv::Point>>::iterator it1 = contours1.begin(); it1 != contours1.end(); it1++){
	glBegin(GL_TRIANGLE_STRIP);
	for(std::vector<cv::Point>::iterator it2 = it1->begin(); it2 != it1->end(); it2++){
	  cv::Point p = *it2;
	  for(std::vector<cv::Point>::iterator it22 = it12->begin(); it22 != it12->end(); it22++){
	    cv::Point p2 = *it22;
	    glColor3f (bgr[2],bgr[1],bgr[0]);
	    glVertex3f(p.x/1500.0-0.5, p.y/1500.0-0.5, -height[k]/250.0);
	    glColor3f (bgr2[2],bgr2[1],bgr2[0]);
	    glVertex3f(p2.x/1500.0-0.5, p2.y/1500.0-0.5, -height[k2]/250.0);
	  }
	}
      
	//最後に終端と始点を結ぶ
	std::vector<std::vector<cv::Point>>::iterator it3 = contours1.begin();
	std::vector<cv::Point>::iterator it4 = it1->begin();
	std::vector<std::vector<cv::Point>>::iterator it32 = contours2.begin();
	std::vector<cv::Point>::iterator it42 = it12->begin();
	cv::Point p = *it4;
	cv::Point p2 = *it42;
	glColor3f (bgr[2],bgr[1],bgr[0]);
	glVertex3f(p.x/1500.0-0.5, p.y/1500.0-0.5, -height[k]/250.0);
	glColor3f (bgr2[2],bgr2[1],bgr2[0]);
	glVertex3f(p2.x/1500.0-0.5, p2.y/1500.0-0.5, -height[k2]/250.0);
	//
      
	glEnd();
      }
    }
  }

  
  glFlush ();
}


//-----------------------------------------------------------------------------------
// キーボードのコールバック関数
//-----------------------------------------------------------------------------------
void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 27:
    exit(0);
    break;
  case 's':
    polygon_mode=1;
    break;
  case 'd':
    polygon_mode=0;
    break;
  case 'r':
    if(z<=4){
      z+=1;
      base = (max_h/2.0)/5.0*z;
    }
    break;
  case 'b':
    if(z>=1){
      z-=1;
      base = (max_h/2.0)/5.0*z;
    }
    break;
  default:
    break;
   }
}
//-----------------------------------------------------------------------------------
// マウスクリックのコールバック関数
//-----------------------------------------------------------------------------------
//2D表示(WINDOW0)の方のマウス操作
void mouse0(int button, int state, int x, int y) 
{
   switch (button) {
   case GLUT_LEFT_BUTTON:
     if (state == GLUT_DOWN){
       color=1;
     }
     break;
   case GLUT_MIDDLE_BUTTON:
     if (state == GLUT_DOWN)
       color=0;
     break;
   case GLUT_RIGHT_BUTTON:
     if (state == GLUT_DOWN)
       color=2;
     break;
   default:
     break;
   }
}

//3D表示の方のマウス操作
void mouse1(int button, int state, int x, int y)
{
  mouse_button = button;
  mouse_x = x;	mouse_y = y;

  if(state == GLUT_UP){
    mouse_button = -1;
  }

  glutPostRedisplay();
}

//-----------------------------------------------------------------------------------
// マウス移動のコールバック関数
//-----------------------------------------------------------------------------------
void motion(int x, int y)
{
  switch(mouse_button){
  case GLUT_LEFT_BUTTON://マウス左ボタン

    if( x == mouse_x && y == mouse_y )
      return;

    yaw -= (GLfloat) (x - mouse_x) / 10.0;
    pitch -= (GLfloat) (y - mouse_y) / 10.0;

    break;

  case GLUT_RIGHT_BUTTON://マウス右ボタン

    if( y == mouse_y )
      return;

    if( y < mouse_y )
      distance += (GLfloat) (mouse_y - y)/50.0;
    else
      distance -= (GLfloat) (y - mouse_y)/50.0;

    if( distance > -1.0 ) distance = -1.0;
    if( distance < -10.0 ) distance = -10.0;

    break;
  }

  mouse_x = x;
  mouse_y = y;

  glutPostRedisplay();
}

//-----------------------------------------------------------------------------------
// アイドル時のコールバック関数
//-----------------------------------------------------------------------------------
void idle()
{
  for(int i=0; i<2; i++){
    glutSetWindow(WindowID[i]);
    glutPostRedisplay();
  }
}



int main(int argc, char** argv)
{
  c = argc;
  _argv = argv[1];
  cv_process(argc, argv); //これは必須(グローバル変数を設定するから)
  cv::Mat src_img = cv::imread(_argv, CV_LOAD_IMAGE_COLOR);
  x_len = src_img.cols;
  y_len = src_img.rows;
  glutInit(&argc, argv);
  glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize (900.0, y_len*900.0/x_len);
  glutInitWindowPosition (50, 50);
  WindowID[0] = glutCreateWindow ("Line Recognition");
  init ();
  glutReshapeFunc (reshape);
  glutDisplayFunc(display0);
  glutKeyboardFunc (keyboard);
  glutMouseFunc(mouse0);

  glutInitDisplayMode ( GLUT_SINGLE | GLUT_RGBA | GLUT_DEPTH);  
  glutInitWindowSize (950, 950);
  glutInitWindowPosition (1500, 50);
  WindowID[1] = glutCreateWindow ("Virtual Mountain");  

  init();

  glutDisplayFunc(display1);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse1);
  glutMotionFunc(motion);

  glutIdleFunc(idle);
  glutMainLoop();
  return 0; 
}
