#ifndef FTWORKER_H
#define FTWORKER_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include "fid.h"
#include <gsl/gsl_fft_real.h>
#include <QPair>

/*!
 \brief Class that handles processing of FIDs

 This class uses algorithms from the GNU Scientific library to perform Fast Fourier Transforms on Fid objects.
 The FFT algorithms are based on a mixed-radix approach that is particularly efficient when the Fid length can be factored many times into multiples of 2, 3, and 5.
 Further details about the algorithm can be found at http://www.gnu.org/software/gsl/manual/html_node/Mixed_002dradix-FFT-routines-for-real-data.html.
 In addition, an Fid can be processed by high pass filtering, exponential filtering, and truncation.

 The FtWorker is designed to operate in its own thread of execution.
 Communication can proceed either through signals and slots, or by direct function calls if the calling thread is not the UI thread (e.g., in a BatchManager object).
 Two slots are available, doFt() and filterFid(), and each has a corresponding signal ftDone() and fidDone() that are emitted when the operation is complete.
 Both slots also return the values that are emitted for use in direct function calls; doFt() calls filterFid() internally, for example.

 Fid operations return an Fid object, while FT operations actually return multiple items.
 The doFt() function returns a pair of values, the FT data in XY format, and the maximum magnitude in the FT.
 The latter quantity is used, for instance, to update the display on the hardware control panel of the UI.

 Filtering settings are set by calling setDelay() (initial truncation in microseconds), setHpf() (high pass filter cutoff frequency in kHz), and setExp() (exponential decay filter time constant in microseconds), and the current values can be obtained from the delay(), hpf(), and exp() access functions.
 These values are stored in the internal variables d_delay, d_hpf, and d_exp.
 If any of these values is set to 0, then the operation is not applied.

*/
class FtWorker : public QObject
{
	Q_OBJECT
public:
	/*!
	 \brief Constructor. Does nothing

	 \param parent
	*/
	explicit FtWorker(QObject *parent = nullptr);
	~FtWorker();

signals:
	/*!
	 \brief Emitted when FFT is complete

	 \param ft FT data in XY format
	 \param max Maximum Y value of FT
	*/
	void ftDone(QVector<QPointF> ft, double max);
	/*!
	 \brief Emitted when Fid filtering is complete

	 \param fid The filtered Fid
	*/
	void fidDone(QVector<QPointF> fid);

public slots:
	/*!
	 \brief Filters and performs FFT operation on an Fid

	 The Fid will be filtered and optionally padded according to current settings

	 \param fid Fid to analyze
	 \return QPair<QVector<QPointF>, double> Resulting FT magnitude spectrum in XY format and maximum Y value
	*/
	QPair<QVector<QPointF>,double> doFT(const Fid fid);

	QVector<QPointF> doFT_noPad(const Fid fid, bool offsetOnly = false);
	QVector<QPointF> doFT_pad(const Fid fid, bool offsetOnly = false);

	//function that actually calculates the FT!
	//wt and ws must already be allocated
	QVector<QPointF> calculateFT(const Fid fid, int realPoints, gsl_fft_real_wavetable *wt, gsl_fft_real_workspace *ws, bool offsetOnly = false);

	/*!
	 \brief Perform truncation, high-pass, and exponential filtering on an Fid

	 \param f Fid to filter
	 \return Fid Filtered Fid
	*/
    Fid filterFid(const Fid f);

    Fid padFid(const Fid f);

    void makeWinf(int n);

	/*!
	 \brief Sets initial truncation

	 \param d Time to truncate, in microseconds
	*/
	void setDelay(const double d){ d_delay = d; }
	/*!
	 \brief Sets high-pass filter cutoff frequency

	 \param d Cutoff frequency, in kHz
	*/
	void setHpf(const double d){ d_hpf = d; }
	/*!
	 \brief Sets exponential decay time constant

	 \param d Time constant in microseconds
	*/
	void setExp(const double d){ d_exp = d; }

	void setAutoPad(bool b){ d_autoPadFids = b; }
	void setRemoveDC(bool b){ d_removeDC = b; }
    void setUseWindow(bool b){ d_useWindow = b; }
	/*!
	 \brief Access function for truncation

	 \return double Truncation time in microseconds
	*/
	double delay() const { return d_delay; }
	/*!
	 \brief Access function for high-pass cutoff frequency

	 \return double Cutoff frequency in kHz
	*/
	double hpf() const { return d_hpf; }
	/*!
	 \brief Access function for exponential filter time constant

	 \return double Time constant in microseconds
	*/
	double exp() const { return d_exp; }

	bool autoPad() const { return d_autoPadFids; }
	bool removeDC() const { return d_removeDC; }
    bool isUseWindow() const { return d_useWindow; }

private:
	gsl_fft_real_wavetable *real; /*!< Wavetable for GNU Scientific Library FFT operations */
	gsl_fft_real_workspace *work; /*!< Memory for GNU Scientific Library FFT operations */
	int d_numPnts; /*!< Number of points used to allocate last wavetable and workspace */

	gsl_fft_real_wavetable *realPadded; /*!< Wavetable for GNU Scientific Library FFT operations (with zero padding) */
	gsl_fft_real_workspace *workPadded; /*!< Memory for GNU Scientific Library FFT operations (with zero padding) */
	int d_numPntsPadded; /*!< Number of points used to allocate last wavetable and workspace (with zero padding) */


	double d_delay; /*!< Truncation applied to FIDs, in microseconds */
	double d_hpf; /*!< High-pass filter cutoff frequency applied to FIDs, in kHz */
	double d_exp; /*!< Exponential decay filter time constant applied to FIDs, in microseconds */

	bool d_autoPadFids;
	bool d_removeDC;
	double d_lastMax;
    bool d_useWindow;
    QVector<double> d_winf;

};

#endif // FTWORKER_H
