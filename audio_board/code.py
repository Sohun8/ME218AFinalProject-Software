import board
import time
import audiomixer
import audiocore
import audiomp3
import audiopwmio
import digitalio

pin1=digitalio.DigitalInOut(board.GP2)
pin2=digitalio.DigitalInOut(board.GP3)
pin1.pull=digitalio.Pull.DOWN
pin2.pull=digitalio.Pull.DOWN

audio = audiopwmio.PWMAudioOut(board.GP0)

ding = audiocore.WaveFile(open("ding.wav", "rb"))
wrong = audiocore.WaveFile(open("wrong.wav", "rb"))

musicMP3=open("short_starman.mp3", "rb")
music = audiomp3.MP3Decoder(musicMP3)

mixer = audiomixer.Mixer(voice_count=2, sample_rate=16000, channel_count=1, bits_per_sample=16, samples_signed=True)

audio.play(mixer)

lastSongValue=pin1.value+pin2.value*2

while True:
	songValue=pin1.value+pin2.value*2
	if(songValue!=lastSongValue):
		time.sleep(0.05)
		if(songValue==pin1.value+pin2.value*2):
			if(songValue==1):
				if(mixer.voice[0].playing):
					mixer.voice[0].stop()
					musicMP3.seek(0)
					music = audiomp3.MP3Decoder(musicMP3)
					time.sleep(0.1)
					mixer.voice[0].play(music)					
				else:
					mixer.voice[0].play(music)
			if(songValue==2):
				mixer.voice[1].play( ding )
			if(songValue==3):
				mixer.voice[1].play( wrong )
			lastSongValue=songValue
