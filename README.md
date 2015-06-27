# LAST
by 1032 Practical Linux System Administration Final Report Group 10 -  賴原群 <100321003>，梁瑜芳<101213035>

For the Graphic Version, pls go to the following link:[slide](http://www.slideshare.net/HazelLiang2/last-49903698)


###about LAST 
*   Light-weight
*   Adjusteable
*   Surveillance
*   Tool

###實作所需材料
| 來源     |      品名              | 價錢（NT）|
|----------|:----------------------:| ---------:|
| 課程提供 |  raspbery pi           |  //       |        
| 校園團購 |    攝影鏡頭            |  $847     |      
| 網路     | SG90 9G伺服器＊2       |  $54.8    |        
| 網路     | 公對母40條杜邦線(6used)|  $54.8    |
| 網路     | usb-to-ttl             |  $30      |        
| 網路     | 平移/傾斜伺服器支架    |  $250     |        

###使用的現有軟體與來源
|   項目       |       來源                      |
|--------------|:-------------------------------:| 
| 影像處理     | OpenCV                          |         
| 校園團購     | vlc(for test),h264_rtspserver   |        
| 馬達控制     | ncurses(surface),pigpio         |          
| 連線及多線程 | socket,pthread                  |  

###實作過程
1.  影像處理程式直接在rpi上執行，CPU使用率達100%，有嚴重延遲現象

  [sol] 於是我們將影像從rpi以串流的方式傳出，由更高效能的電腦接手處理
2.  旋轉圖像時邊緣會被切到
  
  [sol] 計算旋轉後圖形框架，再將圖像平移到框架中心點
    ![rotateimg](https://raw.githubusercontent.com/NCNU-OpenSource/LAST/master/images/last-rotateimg.jpg)

3.  調整hsv時發生overflow

  [sol] 處理邊界值
  
    <img src="https://github.com/NCNU-OpenSource/LAST/blob/master/images/last-hsv.png?raw=true/LAST/master/images/last-hsv.jpg" alt="rotateimg" height="360" width="480">
    <img src="https://github.com/NCNU-OpenSource/LAST/blob/master/images/last-rgb.png?raw=true/LAST/master/images/last.rgb.jpg" alt="rotateimg" height="240" width="240">
4.  抓取鏡頭影像時，起出無法及OpenCV抓取，於是我們

  [sol.1] 另外抓取rpi camera影像的library

  [sol.2] 後來載入driver(bcm2835-v412)可直接使用OpenCV來抓取影像
  
5.  馬達控制
  - 選擇馬達
    \>伺服馬達
  - 利用PWM控制伺服馬達：發送週期為20ms的PWM訊號
  - use pigpio. 有專用於伺服馬達控制的API  (vs. writingpi,bcm2835提供之PWM API 僅能控制一個gpio輸出PWM訊號)
  
###實際產出
![end product](https://github.com/NCNU-OpenSource/LAST/blob/master/images/IMG_2826.JPG?raw=true "end product")


### Compilation

##### Required Libraries

1. ncureses

    `sudo apt-get install libncurses5-dev`

2. OpenCV

    [Installation in Linux](http://docs.opencv.org/doc/tutorials/introduction/linux_install/linux_install.html#linux-installation)

3. pigpio

    [Download & Install](http://abyz.co.uk/rpi/pigpio/download.html)

* For directories contain `CMakeLists.txt`

    `cd `***directory***

    `cmake .`

    `make`

* Others

    `cd `***directory***

    `make`


###Assignments
影像處理：kent, nico
馬達控制：kent
備材：kent
組裝：kent, nico
文件：nico
報告：kent,nico

###Referencs

