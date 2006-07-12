/******************************************************************************
 *
 *  $Id$
 *
 *****************************************************************************/

#ifndef DLSRingBufferTHpp
#define DLSRingBufferTHpp

/*****************************************************************************/

#include <string>
using namespace std;

/*****************************************************************************/

/**
   Ringpuffer f�r beliebige Datentypen

   Das Template ben�tigt f�rt die Spezialisierung zwei
   Datentypen: TYPE f�r den Typ der Puffer-Elemente
   uns SIZE f�r den Typ der Puffergr��e.
   "unsigned int" sollte hier in den meisten F�llen die
   vern�nftigste Wahl sein.
*/

template <class TYPE, class SIZE>
class COMRingBufferT
{
public:
    COMRingBufferT(SIZE);
    ~COMRingBufferT();

    TYPE &operator[](SIZE);
    const TYPE &operator[](SIZE) const;

    SIZE length() const;
    void clear();
    void erase_first(SIZE);

    void write_info(TYPE **, SIZE *);
    void written(SIZE);

private:
    TYPE *_buffer;
    SIZE _index;
    SIZE _length;
    SIZE _size;
};

/*****************************************************************************/

/**
   Konstruktor

   \para size Gr��e des Rings
*/

template <class TYPE, class SIZE>
COMRingBufferT<TYPE, SIZE>::COMRingBufferT(SIZE size)
{
    _buffer = 0;
    _index = 0;
    _length = 0;
    _size = 0;

    _buffer = new TYPE[size];
    _size = size;
}

/*****************************************************************************/

/**
   Destruktor
*/

template <class TYPE, class SIZE>
COMRingBufferT<TYPE, SIZE>::~COMRingBufferT()
{
    if (_size) delete [] _buffer;
}

/*****************************************************************************/

/**
   Index-Operator: Lese- und Schreibzugriff auf ein Puffer-Element

   Wird ein Index angegeben, der die Ringgr��e �berschreitet,
   wird eine Modulo-Operation durchgef�hrt. Es kann also keine
   Bereichsverletzung geben.

   \param index Index des Puffer-Elementes
   \return Referenz auf das Puffer-Element
*/

template <class TYPE, class SIZE>
TYPE &COMRingBufferT<TYPE, SIZE>::operator[](SIZE index)
{
    if ((_index + index) < _size)
    {
        return _buffer[_index + index];
    }
    else
    {
        return _buffer[(_index + index) % _size];
    }
}

/*****************************************************************************/

/**
   Index-Operator: Konstante Version

   Siehe operator[]

   \param index Index des Puffer-Elementes
   \return Konstante Referenz auf das Puffer-Element
*/

template <class TYPE, class SIZE>
const TYPE &COMRingBufferT<TYPE, SIZE>::operator[](SIZE index) const
{
    if (index < _size)
    {
        return _buffer[index];
    }
    else
    {
        return _buffer[index % _size];
    }
}

/*****************************************************************************/

/**
   Gr��e des aktuellen Ring-Inhalts

   \return Anzahl der Werte im Ring
*/

template <class TYPE, class SIZE>
inline SIZE COMRingBufferT<TYPE, SIZE>::length() const
{
    return _length;
}

/*****************************************************************************/

/**
   Leert den Ring
*/

template <class TYPE, class SIZE>
inline void COMRingBufferT<TYPE, SIZE>::clear()
{
    _length = 0;
}

/*****************************************************************************/

/**
   L�scht die ersten n Elemente

   Ist n gr��er als der aktuelle Inhalt, wird der gesamte
   Ringinhalt gel�scht.

   \param n Anzahl der zu l�schenden Elemente
*/

template <class TYPE, class SIZE>
void COMRingBufferT<TYPE, SIZE>::erase_first(SIZE n)
{
    if (n < _length)
    {
        _index = (_index + n) % _size;
        _length -= n;
    }
    else
    {
        clear();
    }
}

/*****************************************************************************/

/**
   Liefert Informationen �ber aktuellen Schreibzeiger und -L�nge

   Kann z. B. f�r "read()" verwendet werden:

   COMRingBufferT<char, unsigned int> ring;
   char *write_ptr;
   unsigned int size;
   ring.write_info(&write_ptr, &size);
   bytes = read(fd, write_ptr, size);
   ring.written(bytes);

   WICHTIG! Nach jedem, manuellen Schreiben an die
   zur�ckgelieferte Adresse muss die Methode written()
   aufgerufen werden, damit die �nderungen �bernommen werden.
*/

template <class TYPE, class SIZE>
void COMRingBufferT<TYPE, SIZE>::write_info(TYPE **addr, SIZE *size)
{
    SIZE write_index = (_index + _length) % _size;
    SIZE remaining = _size - _length;

    // Bei �bertretung der Ringgrenze abschneiden
    if (write_index + remaining > _size)
    {
        remaining = _size - write_index;
    }

    *addr = _buffer + write_index;
    *size = remaining;
}

/*****************************************************************************/

/**
   Teilt dem Ring mit, dass manuell Daten in den
   Puffer geschrieben wurden

   \param length Anzahl der Werte, die geschrieben wurden
*/

template <class TYPE, class SIZE>
void COMRingBufferT<TYPE, SIZE>::written(SIZE length)
{
    _length += length;
}

/*****************************************************************************/

#endif
