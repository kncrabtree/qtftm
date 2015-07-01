#ifndef DOPPLERPAIR_H
#define DOPPLERPAIR_H

#include <QObject>
#include <qwt6/qwt_plot_marker.h>
#include <qwt6/qwt_plot_picker.h>

/*!
 \brief Represents a pair of frequencies on a plot

 The DopplerPair is a set of two frequencies with a center frequency (d_center) and a splitting relative to the center (d_splitting).
 For representation on a plot, there are a pair of QwtPlotMarker objects, called the redMarker (d_center - d_splitting) and the bluemarker (d_center + d_splitting).
 These markers can be attached and detached from a plot with the attach() and detach() functions, and their X values are updated internally gwith the updateMarkers() function.
 When multiple doppler pairs are shown on a plot, each has its own number (num) which serves as an index.
 The index is represented with a number that is placed next to the marker on the plot.

 Generally, these are just used for display purposes, but the values of the number, center, and splitting are accessible through the public accessor functions number(), center(), and splitting().
 There is a convenience function toString(), which gives a string formatted like "num: d_center +/- d_splitting" for display purposes.

*/
class DopplerPair : public QObject
{
	Q_OBJECT
public:
	/*!
	 \brief Constructor. Sets initial values of num, d_center, and d_splitting, and initializes markers

	 \param n Number
	 \param c Center frequency (MHz)
	 \param s Splitting (MHz)
	 \param parent Parent object
	*/
    explicit DopplerPair(int n = 0, double c = 0.0, double s = 0.0, double a = 0.0, QObject *parent = nullptr);

	/*!
	 \brief Returns string representation of pair

	 \return const QString "num: d_center +/- d_splitting"
	*/
	const QString toString() const;
	/*!
	 \brief Attaches redMarker and blueMarker to the indicated plot

	 See QwtPlotItem::attach()

	 \param p Pointer to the plot.
	*/
	void attach(QwtPlot *p);
	/*!
	 \brief Removes redMarker and blueMarker from any plots

	 See QwtPlotItem::detach()

	*/
	void detach();

	/*!
	 \brief Accessor function for center frequency

	 \return double Center frequency (MHz)
	*/
	double center() const { return d_center; }
	/*!
	 \brief Accessor function for splitting

	 \return double Splitting (MHz)
	*/
	double splitting() const { return d_splitting; }
	/*!
	 \brief Accessor function for number

	 \return int Index number
	*/
	int number() const { return num; }

    double amplitude() const { return d_amplitude; }

    void setAmplitude(double d) { d_amplitude = d; }
	
signals:
	
public slots:
	/*!
	 \brief Changes center frqeuency, and updates markers

	 \param newPos New center frequency (MHz)
	*/
	void moveCenter(double newPos);
	/*!
	 \brief Changes splitting, and updates markers

	 \param newSplit New splitting (MHz)
	*/
	void split(double newSplit);
	/*!
	 \brief Changes number, and updates markers

	 \sa makeMarkerText()

	 \param newNum New index number
	*/
	void changeNumber(int newNum);

private:
	int num; /*!< Index number */

	QwtPlotMarker redMarker; /*!< Marker for d_center - d_splitting */
	QwtPlotMarker blueMarker; /*!< Marker for d_center + d_splitting */

	double d_center; /*!< Center frequency (MHz) */
	double d_splitting; /*!< Splitting (MHz) */
    double d_amplitude;

	/*!
	 \brief Sets X values for redMarker and blueMarker

	*/
	void updateMarkers();

	/*!
	 \brief Creates the text that shows the index number on the plot

	 The string has newlines so that there is no overlap between text from different markers.
	 The number of newlines is equal to the index number.
	 Text is shown just to the left of the marker on the plot.

	 \return QString
	*/
	QString makeMarkerText();

	
};

#endif // DOPPLERPAIR_H
