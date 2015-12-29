ALSA_MIDI

キーボードの入力をMIDI信号に変えて送信するプログラム。
入力キーと出力MIDI番号の対応は、setting.txtに書きます。
Raspberry Pi で動かすことを想定していますが、普通のUbuntu等でも動きます。


コンパイル方法:

必要なライブラリのインストール
$ sudo apt-get install libasound2-dev

コンパイル
$ gcc alsa_midi.c -lasound -o alsa_midi




ファイルの説明：

alsa_midi_image.img		SDカードのイメージファイル。ddコマンド等で直接SDカードにコピーして使うことができる。

alsa_midi.c		プログラムのソースコード
setting.txt		設定ファイル

install手順		Raspberry Pi へのインストール方法。自動起動の設定も含む。




RaspberryPi用SDカードの復旧方法：

SDカードが壊れた場合、alsa_midi_image.imgをddコマンド等でコピーすることで作り直すことができます。

$ sudo dd bs=4M if=alsa_midi_image.img of={SDカードのデバイス名}

{SDカードのデバイス名}は、例えば/dev/mmcblk0などです。

