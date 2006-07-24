// sound.cpp	-- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Code to handle SWF sound-related tags.


#include "sound.h"
#include "stream.h"
#include "impl.h"
#include "log.h"
#include "execute_tag.h" // for start_sound_tag inheritance
#include "movie_definition.h"

namespace gnash {
	// Callback interface to host, for handling sounds.  If it's NULL,
	// sound is ignored.
	static sound_handler*	s_sound_handler = 0;


	void	set_sound_handler(sound_handler* s)
	// Called by host, to set a handler for all sounds.
	// Can pass in 0 to disable sound.
	{
		s_sound_handler = s;
	}


	sound_handler*	get_sound_handler()
	{
		return s_sound_handler;
	}


	sound_sample_impl::~sound_sample_impl()
	{
		if (s_sound_handler)
		{
			s_sound_handler->delete_sound(m_sound_handler_id);
		}
	}


	// Utility function to uncompress ADPCM.
	static void	adpcm_expand(
		void* data_out,
		stream* in,
		int sample_count,	// in stereo, this is number of *pairs* of samples
		bool stereo);



	/// SWF Tag StartSound (15) 
	struct start_sound_tag : public execute_tag
	{
		uint16_t	m_handler_id;
		int	m_loop_count;
		bool	m_stop_playback;

		start_sound_tag()
			:
			m_handler_id(0),
			m_loop_count(0),
			m_stop_playback(false)
		{
		}


		void	read(stream* in, int tag_type, movie_definition* m, const sound_sample_impl* sam)
		// Initialize this StartSound tag from the stream & given sample.
		// Insert ourself into the movie.
		{
			assert(sam);

			in->read_uint(2);	// skip reserved bits.
			m_stop_playback = in->read_uint(1) ? true : false;
			bool	no_multiple = in->read_uint(1) ? true : false;
			bool	has_envelope = in->read_uint(1) ? true : false;
			bool	has_loops = in->read_uint(1) ? true : false;
			bool	has_out_point = in->read_uint(1) ? true : false;
			bool	has_in_point = in->read_uint(1) ? true : false;

			UNUSED(no_multiple);
			UNUSED(has_envelope);
			
			uint32_t	in_point = 0;
			uint32_t	out_point = 0;
			if (has_in_point) { in_point = in->read_u32(); }
			if (has_out_point) { out_point = in->read_u32(); }
			if (has_loops) { m_loop_count = in->read_u16(); }
			// if (has_envelope) { env_count = read_uint8(); read envelope entries; }

			m_handler_id = sam->m_sound_handler_id;
			m->add_execute_tag(this);
		}


		void	execute(movie* m)
		{
			if (s_sound_handler)
			{
				if (m_stop_playback)
				{
					s_sound_handler->stop_sound(m_handler_id);
				}
				else
				{
					s_sound_handler->play_sound(m_handler_id, m_loop_count, 0);
				}
			}
		}
	};

#ifdef HAVE_GST_GST_H
	// Used to simulate a start_sound_tag when we have a stream
	struct start_stream_sound_tag : public execute_tag
	{
		uint16_t	m_handler_id;
		int	m_loop_count;
		bool	m_stop_playback;

		start_stream_sound_tag()
			:
			m_handler_id(0),
			m_loop_count(0),
			m_stop_playback(false)
		{
		}


		void	read(movie_definition* m, const sound_sample_impl* sam)
		// Initialize this StartSound tag from the stream & given sample.
		// Insert ourself into the movie.
		{
			assert(sam);

			m_handler_id = sam->m_sound_handler_id;
			m->add_execute_tag(this);
		}


		void	execute(movie* m)
		{
			if (s_sound_handler)
			{
				if (m_stop_playback)
				{
					s_sound_handler->stop_sound(m_handler_id);
				}
				else
				{
					s_sound_handler->play_sound(m_handler_id, m_loop_count, 0);
				}
			}
		}
	};
#endif


	// void	define_button_sound(...) ???


// @@ currently not implemented
//	void	sound_stream_loader(stream* in, int tag_type, movie_definition* m)
//	// Load the various stream-related tags: SoundStreamHead,
//	// SoundStreamHead2, SoundStreamBlock.
//	{
//	}


	//
	// ADPCM
	//


	// Data from Alexis' SWF reference
	static int	s_index_update_table_2bits[2] = { -1,  2 };
	static int	s_index_update_table_3bits[4] = { -1, -1,  2,  4 };
	static int	s_index_update_table_4bits[8] = { -1, -1, -1, -1,  2,  4,  6,  8 };
	static int	s_index_update_table_5bits[16] = { -1, -1, -1, -1, -1, -1, -1, -1, 1,  2,  4,  6,  8, 10, 13, 16 };

	static int*	s_index_update_tables[4] = {
		s_index_update_table_2bits,
		s_index_update_table_3bits,
		s_index_update_table_4bits,
		s_index_update_table_5bits,
	};

	// Data from Jansen.  http://homepages.cwi.nl/~jack/
	// Check out his Dutch retro punk songs, heh heh :)
	const int STEPSIZE_CT = 89;
	static int s_stepsize[STEPSIZE_CT] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
		19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
		50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
		130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
		2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
		5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};


	// Algo from http://www.circuitcellar.com/pastissues/articles/richey110/text.htm
	// And also Jansen.
	// Here's another reference: http://www.geocities.com/SiliconValley/8682/aud3.txt
	// Original IMA spec doesn't seem to be on the web :(


	// @@ lots of macros here!  It seems that VC6 can't correctly
	// handle integer template args, although it's happy to
	// compile them?!

//	void DO_SAMPLE(int n_bits, int& sample, int& stepsize_index, int raw_code)
#define DO_SAMPLE(n_bits, sample, stepsize_index, raw_code)									\
	{															\
		assert(raw_code >= 0 && raw_code < (1 << n_bits));								\
																\
		static const int	HI_BIT = (1 << (n_bits - 1));								\
		int*	index_update_table = s_index_update_tables[n_bits - 2];							\
																\
		/* Core of ADPCM. */												\
																\
		int	code_mag = raw_code & (HI_BIT - 1);									\
		bool	code_sign_bit = (raw_code & HI_BIT) ? 1 : 0;								\
		int	mag = (code_mag << 1) + 1;	/* shift in LSB (they do this so that pos & neg zero are different)*/	\
																\
		int	stepsize = s_stepsize[stepsize_index];									\
																\
		/* Compute the new sample.  It's the predicted value			*/					\
		/* (i.e. the previous value), plus a delta.  The delta			*/					\
		/* comes from the code times the stepsize.  going for			*/					\
		/* something like: delta = stepsize * (code * 2 + 1) >> code_bits	*/					\
		int	delta = (stepsize * mag) >> (n_bits - 1);								\
		if (code_sign_bit) delta = -delta;										\
																\
		sample += delta;												\
		sample = iclamp(sample, -32768, 32767);										\
																\
		/* Update our stepsize index.  Use a lookup table. */								\
		stepsize_index += index_update_table[code_mag];									\
		stepsize_index = iclamp(stepsize_index, 0, STEPSIZE_CT - 1);							\
	}


	struct in_stream
	{
		const unsigned char*	m_in_data;
		int	m_current_bits;
		int	m_unused_bits;

		in_stream(const unsigned char* data)
			:
			m_in_data(data),
			m_current_bits(0),
			m_unused_bits(0)
		{
		}
	};


//	void DO_MONO_BLOCK(int16_t** out_data, int n_bits, int sample_count, stream* in, int sample, int stepsize_index)
#define DO_MONO_BLOCK(out_data, n_bits, sample_count, in, sample, stepsize_index)						\
	{															\
		/* First sample doesn't need to be decompressed. */								\
		sample_count--;													\
		*(*out_data)++ = (int16_t) sample;										\
																\
		while (sample_count--)												\
		{														\
			int	raw_code = in->read_uint(n_bits);								\
			DO_SAMPLE(n_bits, sample, stepsize_index, raw_code);	/* sample & stepsize_index are in/out params */	\
			*(*out_data)++ = (int16_t) sample;									\
		}														\
	}


//	void do_stereo_block(
//		int16_t** out_data,	// in/out param
//		int n_bits,
//		int sample_count,
//		stream* in,
//		int left_sample,
//		int left_stepsize_index,
//		int right_sample,
//		int right_stepsize_index
//		)
#define DO_STEREO_BLOCK(out_data, n_bits, sample_count, in, left_sample, left_stepsize_index, right_sample, right_stepsize_index) \
	/* Uncompress 4096 stereo sample pairs of ADPCM. */									  \
	{															  \
		/* First samples don't need to be decompressed. */								  \
		sample_count--;													  \
		*(*out_data)++ = (int16_t) left_sample;										  \
		*(*out_data)++ = (int16_t) right_sample;										  \
																  \
		while (sample_count--)												  \
		{														  \
			int	left_raw_code = in->read_uint(n_bits);								  \
			DO_SAMPLE(n_bits, left_sample, left_stepsize_index, left_raw_code);					  \
			*(*out_data)++ = (int16_t) left_sample;									  \
																  \
			int	right_raw_code = in->read_uint(n_bits);								  \
			DO_SAMPLE(n_bits, right_sample, right_stepsize_index, right_raw_code);					  \
			*(*out_data)++ = (int16_t) right_sample;									  \
		}														  \
	}


	void	adpcm_expand(
		void* out_data_void,
		stream* in,
		int sample_count,	// in stereo, this is number of *pairs* of samples
		bool stereo)
	// Utility function: uncompress ADPCM data from in stream to
	// out_data[].	The output buffer must have (sample_count*2)
	// bytes for mono, or (sample_count*4) bytes for stereo.
	{
		int16_t*	out_data = (int16_t*) out_data_void;

		// Read header.
		int	n_bits = in->read_uint(2) + 2;	// 2 to 5 bits

		while (sample_count)
		{
			// Read initial sample & index values.
			int	sample = in->read_sint(16);

			int	stepsize_index = in->read_uint(6);
			assert(STEPSIZE_CT >= (1 << 6));	// ensure we don't need to clamp.

			int	samples_this_block = imin(sample_count, 4096);
			sample_count -= samples_this_block;

			if (stereo == false)
			{
#define DO_MONO(n) DO_MONO_BLOCK(&out_data, n, samples_this_block, in, sample, stepsize_index)

				switch (n_bits)
				{
				default: assert(0); break;
				case 2: DO_MONO(2); break;
				case 3: DO_MONO(3); break;
				case 4: DO_MONO(4); break;
				case 5: DO_MONO(5); break;
				}
			}
			else
			{
				// Stereo.

				// Got values for left channel; now get initial sample
				// & index for right channel.
				int	right_sample = in->read_sint(16);

				int	right_stepsize_index = in->read_uint(6);
				assert(STEPSIZE_CT >= (1 << 6));	// ensure we don't need to clamp.

#define DO_STEREO(n)					\
	DO_STEREO_BLOCK(				\
		&out_data, n, samples_this_block,	\
		in, sample, stepsize_index,		\
		right_sample, right_stepsize_index)
			
				switch (n_bits)
				{
				default: assert(0); break;
				case 2: DO_STEREO(2); break;
				case 3: DO_STEREO(3); break;
				case 4: DO_STEREO(4); break;
				case 5: DO_STEREO(5); break;
				}
			}
		}
	}

namespace SWF {
namespace tag_loaders {

// Load a DefineSound tag.
void
define_sound_loader(stream* in, tag_type tag, movie_definition* m)
{
	assert(tag == 14);

	uint16_t	character_id = in->read_u16();

	sound_handler::format_type	format = (sound_handler::format_type) in->read_uint(4);
	int	sample_rate = in->read_uint(2);	// multiples of 5512.5
	bool	sample_16bit = in->read_uint(1) ? true : false;
	bool	stereo = in->read_uint(1) ? true : false;
	int	sample_count = in->read_u32();

	static int	s_sample_rate_table[] = { 5512, 11025, 22050, 44100 };

	log_parse("define sound: ch=%d, format=%d, rate=%d, 16=%d, stereo=%d, ct=%d\n",
		  character_id, int(format), sample_rate, int(sample_16bit), int(stereo), sample_count);

	// If we have a sound_handler, ask it to init this sound.
	if (s_sound_handler)
	{
		int	data_bytes = 0;
		unsigned char*	data = NULL;

		if (format == sound_handler::FORMAT_ADPCM)
		{
			// Uncompress the ADPCM before handing data to host.
			data_bytes = sample_count * (stereo ? 4 : 2);
			data = new unsigned char[data_bytes];
			adpcm_expand(data, in, sample_count, stereo);
			format = sound_handler::FORMAT_NATIVE16;
		}
		else
		{
			// @@ This is pretty awful -- lots of copying, slow reading.
			data_bytes = in->get_tag_end_position() - in->get_position();
			data = new unsigned char[data_bytes];
			for (int i = 0; i < data_bytes; i++)
			{
				data[i] = in->read_u8();
			}

			// Swap bytes on behalf of the host, to make it easier for the handler.
			// @@ I'm assuming this is a good idea?	 Most sound handlers will prefer native endianness?
			if (format == sound_handler::FORMAT_UNCOMPRESSED
			    && sample_16bit)
			{
				#ifndef _TU_LITTLE_ENDIAN_
				// Swap sample bytes to get big-endian format.
				for (int i = 0; i < data_bytes - 1; i += 2)
				{
					swap(&data[i], &data[i+1]);
				}
				#endif // not _TU_LITTLE_ENDIAN_

				format = sound_handler::FORMAT_NATIVE16;
			}
		}
		
		int	handler_id = s_sound_handler->create_sound(
			data,
			data_bytes,
			sample_count,
			format,
			s_sample_rate_table[sample_rate],
			stereo,
			false);
		sound_sample*	sam = new sound_sample_impl(handler_id);
		m->add_sound_sample(character_id, sam);

		delete [] data;
	}
}


void
start_sound_loader(stream* in, tag_type tag, movie_definition* m)
// Load a StartSound tag.
{
	assert(tag == 15);

	uint16_t	sound_id = in->read_u16();

	sound_sample_impl*	sam = (sound_sample_impl*) m->get_sound_sample(sound_id);
	if (sam)
	{
		start_sound_tag*	sst = new start_sound_tag();
		sst->read(in, tag, m, sam);

		log_parse("start_sound tag: id=%d, stop = %d, loop ct = %d\n",
			  sound_id, int(sst->m_stop_playback), sst->m_loop_count);
	}
	else
	{
		if (s_sound_handler)
		{
			log_error("start_sound_loader: sound_id %d is not defined\n", sound_id);
		}
	}
	
}



// Load a SoundStreamHead(2) tag.
void
sound_stream_head_loader(stream* in, tag_type tag, movie_definition* m)
{
#ifdef HAVE_GST_GST_H
	assert(tag == 18 || tag == 45);

	// FIXME:
	// no character id for soundstreams... so we make one up... 
	// This only works if there is only one stream in the movie...
	// The right way to do it is to make seperate structures for streams
	// in movie_def_impl.
	uint16_t	character_id = 10000;
	
	// extract garbage data
	int	garbage = in->read_uint(8);

	sound_handler::format_type	format = (sound_handler::format_type) in->read_uint(4);
	int	sample_rate = in->read_uint(2);	// multiples of 5512.5
	bool	sample_16bit = in->read_uint(1) ? true : false;
	bool	stereo = in->read_uint(1) ? true : false;
	
	// checks if this is a new streams header or just one in the row
	if (format == 0 && sample_rate == 0 && sample_16bit == 0 && stereo == 0) return;
	
	int	sample_count = in->read_u32();
	if (format == 2) garbage = in->read_uint(16);

	static int	s_sample_rate_table[] = { 5512, 11025, 22050, 44100 };

	log_parse("sound stream head: ch=%d, format=%d, rate=%d, 16=%d, stereo=%d, ct=%d\n",
		  character_id, int(format), sample_rate, int(sample_16bit), int(stereo), sample_count);

	// If we have a sound_handler, ask it to init this sound.
	if (s_sound_handler)
	{
		int	data_bytes = 0;
		unsigned char*	data = NULL;

		int	handler_id = s_sound_handler->create_sound(
			data,
			data_bytes,
			sample_count,
			format,
			s_sample_rate_table[sample_rate],
			stereo,
			true);
		sound_sample*	sam = new sound_sample_impl(handler_id);
		m->add_sound_sample(character_id, sam);

		sound_sample_impl*	sam_impl = (sound_sample_impl*) m->get_sound_sample(10000);
		start_stream_sound_tag*	ssst = new start_stream_sound_tag();
		ssst->read(m, sam_impl);

		delete [] data;
	}
#endif
}


// Load a SoundStreamBlock tag.
void
sound_stream_block_loader(stream* in, tag_type tag, movie_definition* m)
{
#ifdef HAVE_GST_GST_H
	assert(tag == 19);

	// extract garbage data
	int	garbage = in->read_uint(32);

	// If we have a sound_handler, store the data with the appropiate sound.
	if (s_sound_handler)
	{
		int	data_bytes = 0;
		unsigned char*	data = NULL;

		// @@ This is pretty awful -- lots of copying, slow reading.
		data_bytes = in->get_tag_end_position() - in->get_position();
		data = new unsigned char[data_bytes];
		for (int i = 0; i < data_bytes; i++)
		{
			data[i] = in->read_u8();
		}

		// Swap bytes on behalf of the host, to make it easier for the handler.
		// @@ I'm assuming this is a good idea?	 Most sound handlers will prefer native endianness?
		s_sound_handler->fill_stream_data(data, data_bytes);

		delete [] data;
	}
#endif
}


} // namespace gnash::SWF::tag_loaders
} // namespace gnash::SWF

} // namespace gnash


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
