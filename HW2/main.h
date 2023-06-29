/// <summary>
/// For easier address calculation.
/// </summary>
/// <param name="row">less than row1.</param>
/// <param name="col">less than col1.</param>
/// <returns>A[row][col]</returns>
int getA(unsigned row, unsigned col);

/// <summary>
/// For easier address calculation.
/// </summary>
/// <param name="row">less than row1.</param>
/// <param name="col">less than col1.</param>
/// <returns>B[row][col]</returns>
int getB(unsigned row, unsigned col);

/// <summary>
/// For easier address calculation.
/// </summary>
/// <param name="row">less than row2.</param>
/// <param name="col">less than col2.</param>
/// <returns>C[row][col]</returns>
int getC(unsigned row, unsigned col);

/// <summary>
/// For easier address calculation.
/// </summary>
/// <param name="row">less than row2.</param>
/// <param name="col">less than col2.</param>
/// <returns>D[row][col]</returns>
int getD(unsigned row, unsigned col);

/// <summary>
/// For easier address calculation.
/// </summary>
/// <param name="row">less than row1.</param>
/// <param name="col">less than col1.</param>
/// <returns>J[row][col]</returns>
int getJ(unsigned row, unsigned col);

/// <summary>
/// For easier address calculation.
/// </summary>
/// <param name="row">less than row1.</param>
/// <param name="col">less than col2.</param>
/// <returns>K[row][col]</returns>
int getK(unsigned row, unsigned col);

/// <summary>
/// For easier address calculation.
/// </summary>
/// <param name="row">less than row2.</param>
/// <param name="col">less than col2.</param>
/// <returns>L[row][col]</returns>
int getL(unsigned row, unsigned col);

/// <summary>
/// For easier address calculation and reporting the value of the computation.
/// </summary>
/// <param name="row">less than row1</param>
/// <param name="col">less than col1</param>
/// <param name="val">becomes the value of J[row][col]</param>
void setJ(unsigned row, unsigned col, int val);

/// <summary>
/// For easier address calculation and reporting the value of the computation.
/// </summary>
/// <param name="row">less than row1</param>
/// <param name="col">less than col2</param>
/// <param name="val">becomes the value of K[row][col]</param>
void setK(unsigned row, unsigned col, int val);

/// <summary>
/// For easier address calculation and reporting the value of the computation.
/// </summary>
/// <param name="row">less than row1</param>
/// <param name="col">less than col1</param>
/// <param name="val">becomes the value of J[row][col]</param>
void setL(unsigned row, unsigned col, int val);

/// <summary>
/// Routine for calculating J.
/// </summary>
/// <param name="r">= row of J to be computed.</param>
void* sum1(void* r);

/// <summary>
/// Routine for calculating L.
/// </summary>
/// <param name="r">= row of L to be computed.</param>
void* sum2(void* r);

/// <summary>
/// Routine for calculating K.
/// </summary>
/// <param name="r">= row of K to be computed.</param>
void* mult(void* r);
